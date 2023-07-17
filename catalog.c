#include <stdio.h>
#include <stdlib.h>
#ifdef __OpenBSD__
#include <unistd.h>
#include <err.h>
#endif
#include <sqlite3.h>
#include "html.h"

#define UNUSED(x) (void) (x)

/*
 * Called for each row: prints the <li> of a gecko.
 */
int print_product(void *arg, int argc, char **argv, char **col_name) {
	UNUSED(arg);
	UNUSED(argc);
	UNUSED(col_name);
	printf(
		product_template,
		atoi(argv[0]),
		atoi(argv[0]),
		argv[1],
		argv[3],
		argv[3],
		atoi(argv[4]),
		*argv[2] // get the character
	);
	return 0;
}
/*
 * Prints all the geckos in the catalog.
 */
void
print_catalog() {
	sqlite3 *db;

	if (sqlite3_open("geckos.db", &db) != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(EXIT_FAILURE);
	}

	const char *sql = "select * from catalog order by date(born) desc";

	char *err_msg = NULL;

	// TODO: handle failure and fail early.
	if (sqlite3_exec(db, sql, print_product, NULL, &err_msg) != SQLITE_OK ) {
		fprintf(stderr, "Failed to select data: SQL error: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		exit(EXIT_FAILURE);
	}

	sqlite3_close(db);
}

/*
TODO: use a list to store all the geckos in the catalog, to allow early failure,
      and better priviledge separation.
*/

int
main(void) {

#ifdef __APPLE__
	// TODO: sandbox_init() for macos
#endif

#ifdef __OpenBSD__
	// TODO: unveil
	if (pledge("stdio rpath cpath wpath flock fattr", NULL) == -1)
		err(EXIT_FAILURE, "pledge");
#endif

	send_successful_response();

	puts(
		"<!DOCTYPE html>\n"
		"<html lang=\"it\">"
		"<head>\n"
		);

	printf(head_template,
		"La pagina con i gechi",
		"Esposizione gechi in vendita",
		"Gechi, Geco, vendita"
		);

	puts(
		"<body class=\"grass\">\n"

		"<header>\n"
		"<h1 class=\"rough-font\">Geckoleoart 🦎</h1>\n"
		"<p style=\"max-width: 700px;\">I gechi sono in vendita!</p>\n"
		"</header>\n"

		"<main>\n"
		"<h2 class=\"rough-font\">Catalogo</h2>\n"
		"<ul class=\"product-list\">\n"
		);

	print_catalog();

	puts(
		"</ul>\n"
		"</main>\n"

		"<footer>\n"
			"<address>\n"
				"Contattatemi per acquisti:\n"
				"<a href=\"mailto:claudialam99@gmail.com\">mail</a>,\n"
				"<a href=\"https://instagram.com/geckoleoart\">instagram</a>,\n"
				"<a href=\"https://facebook.com/geckoleoart\">facebook</a>.\n"
			"</address>\n"
		"</footer>\n"

		"</body>\n"
		"</html>"
		);

	return EXIT_SUCCESS;
}
