/*
 * weather-station.c : station météo pour Banana Pi M2 Zero.
 *
 * Affiche sur un écran OLED SSD1306 (I2C) la température, l'humidité et la
 * pression d'un BME280, ainsi que l'heure d'une RTC DS1307.
 *
 * Compilation : gcc -Wall -O2 weather-station.c -o weather-station -lssd1306_i2c
 *
 * Usage : weather-station             affiche sur l'écran (boucle infinie)
 *         weather-station --preview   affiche le rendu en ASCII, sans écran
 *
 * Réglages par variables d'environnement (valeur par défaut entre crochets) :
 *   SSD1306_I2C_DEV  bus I2C de l'écran       [/dev/i2c-0]
 *   SSD1306_ADDR     adresse I2C (0x3c/0x3d)  [0x3c]
 *   SSD1306_HEIGHT   hauteur de l'écran       [32]   (32 ou 64)
 *   WS_PERIOD        période en secondes      [1]
 *   SSD1306_VERBOSE  1 = garde les messages de la bibliothèque
 *   WS_SYSFS         racine de sysfs, pour les tests           [/sys]
 */
#define _GNU_SOURCE /* strcasestr, sigaction */

#include <ssd1306_i2c.h>

#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ------------------------------------------------------------------------
 * Police 5x7 : chaque caractère = 7 lignes de 5 caractères ('#' = allumé).
 * Majuscules, chiffres et quelques symboles : aucune police TrueType requise.
 * ---------------------------------------------------------------------- */
#define CHAR_W 5
#define CHAR_H 7

struct glyph {
	char c;
	const char *rows[CHAR_H];
};

static const struct glyph glyphs[] = {
	{ ' ', { ".....", ".....", ".....", ".....", ".....", ".....", "....." } },
	{ ':', { ".....", "..#..", ".....", ".....", ".....", "..#..", "....." } },
	{ '.', { ".....", ".....", ".....", ".....", ".....", "..#..", "....." } },
	{ '%', { "#...#", "....#", "...#.", "..#..", ".#...", "#....", "....." } },
	{ '-', { ".....", ".....", ".....", "#####", ".....", ".....", "....." } },
	{ '0', { ".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###." } },
	{ '1', { "..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###." } },
	{ '2', { ".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####" } },
	{ '3', { "#####", "...#.", "..#..", "...#.", "....#", "#...#", ".###." } },
	{ '4', { "...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#." } },
	{ '5', { "#####", "#....", "####.", "....#", "....#", "#...#", ".###." } },
	{ '6', { "..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###." } },
	{ '7', { "#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..." } },
	{ '8', { ".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###." } },
	{ '9', { ".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.." } },
	{ 'A', { "..#..", ".#.#.", "#...#", "#...#", "#####", "#...#", "#...#" } },
	{ 'C', { ".###.", "#...#", "#....", "#....", "#....", "#...#", ".###." } },
	{ 'E', { "#####", "#....", "#....", "####.", "#....", "#....", "#####" } },
	{ 'H', { "#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
	{ 'I', { ".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###." } },
	{ 'M', { "#...#", "##.##", "#.#.#", "#...#", "#...#", "#...#", "#...#" } },
	{ 'P', { "####.", "#...#", "#...#", "####.", "#....", "#....", "#...." } },
	{ 'R', { "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" } },
	{ 'S', { ".####", "#....", "#....", ".###.", "....#", "....#", "####." } },
	{ 'T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
	{ 'U', { "#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
};

static const char *const *find_glyph(char c)
{
	unsigned int i;

	for (i = 0; i < sizeof(glyphs) / sizeof(glyphs[0]); i++)
		if (glyphs[i].c == c)
			return glyphs[i].rows;
	return glyphs[0].rows; /* espace */
}

/* ------------------------------------------------------------------------
 * Écran : libssd1306 pour l'I2C, framebuffer fourni ici.
 *
 * ssd1306_framebuffer_create() est volontairement évitée : elle charge huit
 * polices TrueType situées à des chemins Debian et échoue si l'une manque, ce
 * qui serait le cas dans une image Yocto minimale. Les fonctions de dessin
 * utilisées ici (put_pixel, clear, get_pixel) n'ont besoin que de width,
 * height, buffer et len.
 * ---------------------------------------------------------------------- */
#define OLED_WIDTH 128
#define LINE_HEIGHT 8

static ssd1306_i2c_t *g_oled;       /* NULL en mode preview */
static ssd1306_framebuffer_t g_fb;

static int fb_init(unsigned int width, unsigned int height)
{
	g_fb.width = width;
	g_fb.height = height;
	g_fb.len = (size_t)width * height / 8;
	g_fb.buffer = calloc(1, g_fb.len);
	return g_fb.buffer ? 0 : -1;
}

static int screen_open(const char *dev, unsigned int addr, unsigned int height,
		       int verbose)
{
	/* Les erreurs d'ouverture et d'initialisation restent visibles. */
	g_oled = ssd1306_i2c_open(dev, addr, OLED_WIDTH, height, NULL);
	if (!g_oled) {
		fprintf(stderr, "Impossible d'ouvrir l'ecran sur %s (adresse 0x%02x)\n",
			dev, addr);
		return -1;
	}
	if (ssd1306_i2c_display_initialize(g_oled) < 0) {
		fprintf(stderr, "Initialisation de l'ecran impossible : est-il branche ?\n");
		ssd1306_i2c_close(g_oled);
		return -1;
	}
	/* La bibliothèque écrit un message INFO à chaque rafraîchissement : on les
	 * envoie vers /dev/null, sauf en mode verbeux. */
	if (!verbose && g_oled->err) {
		FILE *null_fp = fopen("/dev/null", "w");

		if (null_fp)
			g_oled->err->err_fp = null_fp;
	}
	return fb_init(g_oled->width, g_oled->height);
}

static void screen_text(int x, int y, const char *text)
{
	for (; *text != '\0'; text++) {
		const char *const *rows = find_glyph(*text);
		int row, col;

		for (row = 0; row < CHAR_H; row++)
			for (col = 0; col < CHAR_W; col++)
				if (rows[row][col] == '#')
					ssd1306_framebuffer_put_pixel(&g_fb, x + col, y + row, true);
		x += CHAR_W + 1;
	}
}

static int screen_flush(void)
{
	return ssd1306_i2c_display_update(g_oled, &g_fb) < 0 ? -1 : 0;
}

static void screen_dump_ascii(void)
{
	unsigned int x, y;

	for (y = 0; y < g_fb.height; y++) {
		for (x = 0; x < g_fb.width; x++)
			putchar(ssd1306_framebuffer_get_pixel(&g_fb, x, y) ? '#' : '.');
		putchar('\n');
	}
}

/* ------------------------------------------------------------------------
 * Lecture du BME280 (IIO) et de la DS1307 (sysfs).
 * Unités constatées au PW9 : température en milli-°C, pression en kPa,
 * humidité relative en milli-pourcents.
 * ---------------------------------------------------------------------- */
static const char *sysfs_root(void)
{
	const char *root = getenv("WS_SYSFS");

	return root ? root : "/sys";
}

static int read_line(const char *path, char *buf, size_t size)
{
	FILE *f = fopen(path, "r");

	if (!f)
		return -1;
	if (!fgets(buf, size, f)) {
		fclose(f);
		return -1;
	}
	fclose(f);
	buf[strcspn(buf, "\n")] = '\0';
	return 0;
}

static int read_double(const char *path, double *out)
{
	char buf[64], *end;

	if (read_line(path, buf, sizeof(buf)) < 0)
		return -1;
	*out = strtod(buf, &end);
	return end == buf ? -1 : 0;
}

/* Renvoie 0 si le BME280 a été lu, -1 sinon. */
static int read_bme280(double *temp_c, double *hum_pct, double *press_hpa)
{
	char base[256], path[1024], name[64];
	struct dirent *entry;
	double temp, press, hum;
	DIR *dir;

	snprintf(base, sizeof(base), "%s/bus/iio/devices", sysfs_root());
	dir = opendir(base);
	if (!dir)
		return -1;

	while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, "iio:device", 10) != 0)
			continue;
		snprintf(path, sizeof(path), "%s/%s/name", base, entry->d_name);
		if (read_line(path, name, sizeof(name)) < 0 || strcmp(name, "bme280") != 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s/in_temp_input", base, entry->d_name);
		if (read_double(path, &temp) < 0)
			break;
		snprintf(path, sizeof(path), "%s/%s/in_pressure_input", base, entry->d_name);
		if (read_double(path, &press) < 0)
			break;
		/* Selon la version du pilote, le canal s'appelle l'un ou l'autre. */
		snprintf(path, sizeof(path), "%s/%s/in_humidityrelative_input", base,
			 entry->d_name);
		if (read_double(path, &hum) < 0) {
			snprintf(path, sizeof(path), "%s/%s/in_humidity_input", base,
				 entry->d_name);
			if (read_double(path, &hum) < 0)
				break;
		}
		closedir(dir);
		*temp_c = temp / 1000.0;
		*hum_pct = hum / 1000.0;
		*press_hpa = press * 10.0; /* kPa -> hPa */
		return 0;
	}
	closedir(dir);
	return -1;
}

/* La RTC interne du SoC est rtc0 : la DS1307 est donc rtc1 ou plus, d'où la
 * recherche par nom. Renvoie 0 si l'heure et la date ont été lues. */
static int read_rtc(int *h, int *mi, int *s, int *y, int *mo, int *d)
{
	char base[256], path[1024], name[96], text[32];
	struct dirent *entry;
	DIR *dir;

	snprintf(base, sizeof(base), "%s/class/rtc", sysfs_root());
	dir = opendir(base);
	if (!dir)
		return -1;

	while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, "rtc", 3) != 0)
			continue;
		snprintf(path, sizeof(path), "%s/%s/name", base, entry->d_name);
		if (read_line(path, name, sizeof(name)) < 0 || !strcasestr(name, "ds1307"))
			continue;

		snprintf(path, sizeof(path), "%s/%s/time", base, entry->d_name);
		if (read_line(path, text, sizeof(text)) < 0 ||
		    sscanf(text, "%d:%d:%d", h, mi, s) != 3)
			break;
		snprintf(path, sizeof(path), "%s/%s/date", base, entry->d_name);
		if (read_line(path, text, sizeof(text)) < 0 ||
		    sscanf(text, "%d-%d-%d", y, mo, d) != 3)
			break;
		closedir(dir);
		return 0;
	}
	closedir(dir);
	return -1;
}

/* ------------------------------------------------------------------------
 * Programme principal
 * ---------------------------------------------------------------------- */
static volatile sig_atomic_t g_stop;

static void handle_signal(int sig)
{
	(void)sig;
	g_stop = 1;
}

static const char *env_or(const char *name, const char *fallback)
{
	const char *value = getenv(name);

	return value ? value : fallback;
}

/* Efface le framebuffer et y dessine les lignes (majuscules uniquement). */
static void render(unsigned int height)
{
	char line[32];
	double temp, hum, press;
	int h, mi, s, y, mo, d;
	int have_rtc = (read_rtc(&h, &mi, &s, &y, &mo, &d) == 0);
	int row = 0;

	ssd1306_framebuffer_clear(&g_fb);

	if (read_bme280(&temp, &hum, &press) == 0) {
		snprintf(line, sizeof(line), "TEMP %.1f C", temp);
		screen_text(0, row++ * LINE_HEIGHT, line);
		snprintf(line, sizeof(line), "HUM %.1f %%", hum);
		screen_text(0, row++ * LINE_HEIGHT, line);
		snprintf(line, sizeof(line), "PRES %.0f HPA", press);
		screen_text(0, row++ * LINE_HEIGHT, line);
	} else {
		screen_text(0, row++ * LINE_HEIGHT, "TEMP ---");
		screen_text(0, row++ * LINE_HEIGHT, "HUM ---");
		screen_text(0, row++ * LINE_HEIGHT, "PRES ---");
	}

	if (have_rtc)
		snprintf(line, sizeof(line), "TIME %02d:%02d:%02d", h, mi, s);
	else
		snprintf(line, sizeof(line), "TIME --:--:--");
	screen_text(0, row++ * LINE_HEIGHT, line);

	if (height >= 40) { /* écran de 64 lignes : on affiche aussi la date */
		if (have_rtc)
			snprintf(line, sizeof(line), "%04d-%02d-%02d", y, mo, d);
		else
			snprintf(line, sizeof(line), "--------");
		screen_text(0, row++ * LINE_HEIGHT, line);
	}
}

int main(int argc, char **argv)
{
	unsigned int height = strtoul(env_or("SSD1306_HEIGHT", "32"), NULL, 0);
	unsigned int addr = strtoul(env_or("SSD1306_ADDR", "0x3c"), NULL, 0);
	unsigned int period = strtoul(env_or("WS_PERIOD", "1"), NULL, 0);
	const char *dev = env_or("SSD1306_I2C_DEV", "/dev/i2c-0");
	struct sigaction sa;
	int failing = 0;

	if (argc > 1 && strcmp(argv[1], "--preview") == 0) {
		if (fb_init(OLED_WIDTH, height) < 0)
			return 1;
		render(height);
		screen_dump_ascii();
		return 0;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	if (screen_open(dev, addr, height, getenv("SSD1306_VERBOSE") != NULL) < 0)
		return 1;

	fprintf(stderr, "weather-station start (screen %s, 0x%02x, 128x%u)\n",
		dev, addr, height);

	while (!g_stop) {
		render(height);
		if (screen_flush() < 0) {
			if (!failing) /* un seul message par série d'échecs */
				fprintf(stderr, "Screen Lecture is impossible\n");
			failing = 1;
		} else {
			failing = 0;
		}
		sleep(period ? period : 1);
	}

	/* Arrêt propre : on éteint l'affichage avant de quitter. */
	ssd1306_framebuffer_clear(&g_fb);
	screen_flush();
	ssd1306_i2c_close(g_oled);
	return 0;
}
