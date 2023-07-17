#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>
#ifdef __OpenBSD__
#include <unistd.h>
#include <err.h>
#endif
#include <sqlite3.h>
#include "html.h"

// TODO: use stdint.h
// TODO: solve memory leak with strdup, probably a free_gecko is needed

/*
 * Returns the numeber of consecutive digits in a string starting from s.
 */
unsigned int
num_length(char *s) {
	unsigned int len = 0; // NOTE: pay attention to overflows!!
	while (s != NULL) {
		if (isdigit(*s)) {
			len++;
			s++;
		} else {
			break;
		}
	}
	return len;
}

/*
 * Returns the id number contained in the query string given as input. If the
 * string is malformatted it aborts the execution of the program.
 */
unsigned int
parse_id(char *query) {
	if (query == NULL) {
		goto error;
		exit(EXIT_FAILURE);
	} else if (strlen(query) < 4) { // at least "id=" plus a digit
		goto error;
	} else if (query[0] != 'i' || query[1] != 'd' || query[2] != '=') {
		goto error;
	}

	char *id = query+3;
	unsigned int len = num_length(id);
	if (len == 0)
		goto error;
	else {
		id[len] = '\0';
		return atoi(id); // NOTE: here it may cause problems with big numbers
	}

error:
	send_client_error();
	exit(EXIT_FAILURE);
}

/*
 * Gets a gecko from the database with a given id from the database, if the
 * gecko is found returns true, false otherwise.
 */
static bool
get_gecko(sqlite3 *conn, unsigned int id, Gecko *gecko) {
	assert(gecko != NULL);

	const char * const query = "select id, morph, born, sex, price from geckos where id = ?";
	sqlite3_stmt *stmt = NULL;

	if (sqlite3_prepare_v2(conn, query, -1, &stmt, NULL) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	// TODO: check if result is empty, in that case is a bad request from the client
	if (sqlite3_step(stmt) != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return false;
	}

	gecko->id    = sqlite3_column_int(stmt, 0);
	gecko->morph = strdup((const char *) sqlite3_column_text(stmt, 1));
	gecko->born  = strdup((const char *) sqlite3_column_text(stmt, 2));
	gecko->sex   = *sqlite3_column_text(stmt, 3);
	gecko->price = sqlite3_column_int(stmt, 4);

	sqlite3_finalize(stmt);
	return true;
}

/*
 * Prints the the gecko in HTML.
 */
static void
print_gecko(Gecko *gecko) {
	assert(gecko != NULL);

	printf(
		"<a href=\"pic/%d.jpg\">\n"
			"<img alt=\"gecko\" class=\"product-img float-left\" src=\"pic/tiny_%d.jpg\">\n"
		"</a>\n"
		"<p class=\"product-desc\">\n"
			"Morph: %s<br>\n"
			"Nascita: <time datetime=\"%s\">%s</time><br>\n"
			"Prezzo: %d€\n"
			"Sesso: %c\n"
		"</p>\n",
		gecko->id,
		gecko->id,
		gecko->morph,
		gecko->born,
		gecko->born,
		gecko->price,
		gecko->sex
	);
}

/*
 * Gets the parents of a gecko with a given id from the database, if a parent is
 * missing NULL is inserted, if the query fails false is returned, true
 * otherwise.
 */
static bool
get_parents(sqlite3 *conn, const unsigned int id, Gecko *parents[]) {
	assert(conn != NULL);
	assert(parents != NULL);

	const char *query =
		"select *\n" // TODO: remove the asterisc
		"from geckos mother, geckos son\n"
		"where son.id = ? and mother.id = son.mother\n"
		"union\n"
		"select *\n"
		"from geckos father, geckos son\n"
		"where son.id = ? and father.id = son.father\n";
	sqlite3_stmt *stmt = NULL;

	if (sqlite3_prepare_v2(conn, query, -1, &stmt, NULL) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK
		|| sqlite3_bind_int(stmt, 2, id) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return false;
	}

	for (int i = 0; i < 2; i++) {
		if (sqlite3_step(stmt) != SQLITE_ROW) {
			parents[i] = NULL;
			continue;
		} else {
			parents[i]->id       = sqlite3_column_int(stmt, 0);
			parents[i]->morph    = strdup((const char *) sqlite3_column_text(stmt, 1));
			parents[i]->born     = strdup((const char *) sqlite3_column_text(stmt, 3));
			parents[i]->sex      = *sqlite3_column_text(stmt, 2);
			parents[i]->price    = sqlite3_column_int(stmt, 4);
			 // TODO: check if it needs to be negated
			parents[i]->sellable = sqlite3_column_int(stmt, 7);
		}
	}

	return true;
}

/*
 * If presents prints the parents in HTML.
 */
static void
print_parents(Gecko *parents[]) {
	assert(parents != NULL);

	if (parents[0] == NULL && parents[1] == NULL) {
		puts("<p>sconosciuti</p>");
	} else {
		puts("<ul class=\"product-list\">");
		// TODO: test if sellable, in case add class not-sale
		for (int i = 0; i < 2; i++) {
			if (parents[i] != NULL) {
				printf(product_template,
					parents[i]->id,
					parents[i]->id,
					parents[i]->morph,
					parents[i]->born,
					parents[i]->born,
					parents[i]->price,
					parents[i]->sex
				);
			}
		}
		puts("</ul>");
	}
}

static void
print_html_page(Gecko *gecko, Gecko *parents[]) {
	assert(gecko != NULL);
	assert(parents != NULL);

	puts(
		"<!DOCTYPE html>\n"
		"<html lang=\"it\">"
		);

	printf(head_template,
		"La pagina con i gechi",
		"Dettagli geco",
		"Gechi, Geco, vendita"
		);

	puts("<body class=\"grass\">");

	print_gecko(gecko);
	puts("<h2 class=\"rough-font\">Genitori</h2>");
	print_parents(parents);

	puts(
		"</body>\n"
		"</html>"
		);
}

int
main(void) {

#ifdef __OpenBSD__
	// TODO: unveil
	if (pledge("stdio rpath cpath wpath flock fattr", NULL) == -1)
		err(EXIT_FAILURE, "pledge");
#endif

	sqlite3 *db;

	if (sqlite3_open("geckos.db", &db) != SQLITE_OK) {
		send_server_error();
#ifndef NDEBUG
		fprintf(stderr, "unable to open the database\n");
#endif
		sqlite3_close(db);
		return EXIT_FAILURE;
	}

	const unsigned int id = parse_id(getenv("QUERY_STRING"));

	Gecko gecko;
	if (!get_gecko(db, id, &gecko)) {
		send_server_error();
		sqlite3_close(db);
		return EXIT_FAILURE;
	}

	Gecko p1, p2;
	Gecko *parents[2] = {&p1, &p2};
	if (!get_parents(db, id, parents)) {
		send_server_error();
		sqlite3_close(db);
		return EXIT_FAILURE;
	}

	sqlite3_close(db);

#ifdef __OpenBSD__
	if (pledge("stdio", NULL) == -1)
		err(EXIT_FAILURE, "pledge");
#endif

	send_successful_response();
	print_html_page(&gecko, parents);

	return EXIT_SUCCESS;
}
