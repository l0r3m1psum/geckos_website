// should this be an html.c and html.h file?

#include <stdbool.h>

#define DOMAIN "l0r3m1p5um.com"

typedef struct {
	unsigned int id;
	char *morph;
	char *born;
	char sex;
	unsigned int price;
	bool sellable;
} Gecko;

typedef struct GeckoNode {
	Gecko gecko;
	struct GeckoNode *next;
} GeckoNode;

struct GeckosList {
	GeckoNode *head;
	GeckoNode *tail;
};

/*
// a questa funzione voglio passare append a una lista per rendere le operazioni di get parents e print catalog più semplici
void
foreach_row(sqlite3_stmt *stmt, void (*callback)(sqlite3_stmt *)) {
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		callback(stmt);
	}
	sqlite3_finalize(stmt);
}
void print_gecko()
*/

const char *head_template =
	"<head>\n"
		"\t<title>%s</title>\n"

		"\t<meta charset=\"utf-8\">\n"
		"\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
		"\t<meta name=\"theme-color\" content=\"green\">\n"
		"\t<meta name=\"author\" content=\"Diego Bellani\">\n"
		"\t<meta name=\"description\" content=\"%s\">\n"
		"\t<meta name=\"keywords\" content=\"%s\">\n"

		"\t<link rel=\"icon\" href=\"favicon.ico\">\n"
		"\t<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Piedra\">\n"
		"\t<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Roboto\">\n"
		"\t<link rel=\"stylesheet\" href=\"style.css\">\n"
	"</head>\n";

const char *product_template =
	"<li class=\"product-container grass\">\n" // TODO: aggiungere un %s qui per aggiungere la class not-sale
		"<a title=\"Dettagli\" href=\"http://"DOMAIN"/geckos/gecko?id=%d\">\n"
			"<img alt=\"gecko\" class=\"product-img\" src=\"pic/tiny_%d.jpg\">\n"
		"</a>\n"
		"<p class=\"product-desc\">\n"
			"Morph: %s<br>\n"
			"Nascita: <time datetime=\"%s\">%s</time><br>\n"
			"Prezzo: %d€<br>\n"
			"Sesso: %c\n"
		"</p>\n"
	"</li>\n";

void
send_successful_response() {
	puts("Status: 200 OK\r");
	puts("Content-Type: text/html\r");
	puts("\r");
}

void
send_server_error() {
	puts("Status: 500 Internal Server Error\r");
	puts("Content-Type: text/html\r");
	puts("\r");
	puts("<h1>Internal server error</h1>");
}

void
send_client_error() {
	puts("400 Bad Request\r");
	puts("Content-Type: text/html\r");
	puts("\r");
	puts("<h1>Bad request</h1>");
}
