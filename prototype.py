import sqlite3
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from urllib.parse import urlparse
import re

def get_parents(conn, id):
	"""Returns the parent of a gecko with a ginven id."""
	# inner join?
	assert id >= 1
	query = """
	select id, morph, born, price
	from (
		select *
		from geckos mother, geckos son
		where son.id = ? and mother.id = son.mother
		union
		select *
		from geckos father, geckos son
		where son.id = ? and father.id = son.father
		);
	"""
	c = conn.cursor()
	c.execute(query, (id, id))
	return c.fetchall()

def parse_querystring(query):
	"""Returns the value of the id in the query string."""
	# TODO: maybe I can use some more idiomatic exceptions
	query = query.split('&')
	if len(query) != 1:
		raise ValueError("Invalid query string")

	try:
		key, value = query[0].split('=')
	except ValueError:
		raise

	if key != "id":
		raise ValueError("Invalid query string")

	try:
		id = abs(int(value))
	except ValueError:
		raise

	return id

class Handler(SimpleHTTPRequestHandler):

	head = """
	<head>
		<title>{}</title>

		<meta charset="utf-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<meta name="theme-color" content="green">
		<meta name="author" content="Diego Bellani">
		<meta name="description" content="{}">
		<meta name="keywords" content="{}">

		<link rel="icon" href="favicon.ico">
		<link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Piedra">
		<link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto">
		<link rel="stylesheet" href="style.css">
	</head>
	"""

	# to serve from LAN s/localhost/10.0.0.3/
	product = """
	<li class="product-container grass">
		<a title="Dettagli" href="http://localhost/gecko?id={0}">
			<img alt="gecko" class="product-img" src="pic/tiny_{0}.jpg">
		</a>
		<p class="product-desc">
			Morph: {1}<br>
			Nascita: <time datetime="{2}">{2}</time><br>
			Prezzo: {3}€
		</p>
	</li>
	"""

	def send_headers(self):
		self.send_response(200)
		self.send_header('content-type', 'text/html')
		# TODO: add last-modified using the db mtime
		self.end_headers()

	def serve_catalog(self, conn):
		"""Writes html page with the catalog of geckos"""
		self.send_headers()

		self.wfile.write(("""
			<!DOCTYPE html>
			<html lang="it">
			"""
			+ type(self).head.format(
				"La pagina con i gechi",
				"Esposizione gechi in vendita",
				"Gechi, Geco, vendita")
			+
			"""
			<body class="grass">

			<header>
			<h1 class="rough-font">La pagina con i gechi 🦎</h1>
			<p style="max-width: 700px;">I gechi sono in vendita! Lorem ipsum dolor sit
			amet, consectetur adipiscing elit. Aenean quis justo et arcu elementum venenatis
			ac vitae dolor. Proin ut pretium diam. Praesent volutpat commodo est. Integer
			vitae nulla neque. Sed finibus libero eget sem imperdiet mollis. Ut fringilla
			nisi vitae vestibulum facilisis. Nunc dapibus neque quam, ut convallis turpis
			maximus ac. Ut lacinia mi elit, quis ullamcorper dolor vestibulum ac. Maecenas
			mattis laoreet augue quis ultricies. Sed tincidunt scelerisque tincidunt.</p>
			</header>

			<main>
			<h2 class="rough-font">Catalogo</h2>
			<ul class="product-list">
			""").encode()
		)

		for row in conn.execute("select * from catalog order by date(born) desc"):
			self.wfile.write(
				type(self).product
				.format(row[0], row[1], row[3], row[4])
				.encode()
			)

		self.wfile.write("""
			</ul>
			</main>

			<footer>
				<address>
					Contattatemi per acquisti:
					<a href="mailto:me@example.com">mail</a>,
					<a href="https://telegram.me/username">telegram</a>,
					<a href="https://instagram.com/username">instagram</a>,
					<a href="https://facebook.com/username">facebook</a>.
				</address>
			</footer>

			</body>
			</html>
			""".encode()
		)

	def serve_gecko(self, conn, id):
		"""Writes html page with the infortmation about the gecko with given id"""
		self.send_headers()

		self.wfile.write(("""
			<!DOCTYPE html>
			<html lang="it">
			"""
			+ type(self).head.format(
				"La pagina con i gechi",
				"Dettagli geco",
				"Gechi, Geco, vendita")
			+
			"""
			<body class="grass">
			""").encode()
		)

		gecko = conn.execute(
			"select id, morph, born, price from geckos where id = ?", (id,)
			).fetchone()
		self.wfile.write("""
			<a href="pic/{0}.jpg">
				<img alt="gecko" class="product-img float-left" src="pic/tiny_{0}.jpg">
			</a>
			<p class="product-desc">
				Morph: {1}<br>
				Nascita: <time datetime="{2}">{2}</time><br>
				Prezzo: {3}€
			</p>
			""".format(gecko[0], gecko[1], gecko[2], gecko[3]).encode()
		)

		self.wfile.write("""<h2 class="rough-font">Genitori</h2>""".encode())

		# maybe the use of list is unnecessary
		parents = list(get_parents(conn, id))
		if len(parents) == 0:
			self.wfile.write("<p>Genitori sconosciuti</p>".encode())
		else:
			self.wfile.write('<ul class="product-list">'.encode())
			for row in parents:
				self.wfile.write(
					type(self).product
					.format(row[0], row[1], row[2], row[3])
					.encode()
				)
			self.wfile.write("</ul>".encode())

		self.wfile.write("""
			</body>
			</html>
			""".encode()
		)

	def bad_request(self):
		"""Writes the html for a bad request to the server."""
		self.send_response(400)
		self.send_header("content-type", "text/html")
		self.end_headers()

		self.wfile.write("<h1>error 400: bad request</h1>".encode())

	def do_GET(self):
		"""Handles all the GET requests to the server."""

		# TODO: cosa fare se connect fallisce? (eg. tira un eccezione se entro 5
		#       secondi non riesce a connettersi al database (due to locking))
		with sqlite3.connect('geckos.db') as conn:
			url = urlparse(self.path)
			if url.path == "/" or url.path == "/index.html":
				self.serve_catalog(conn)
			elif url.path == "/gecko":
				try:
					id = parse_querystring(url.query)
				except:
					self.bad_request()
				else:
					self.serve_gecko(conn, id)
			# "/pic?id=1" would be the path to extract pictures from the database
			# "/gallery?rowid=1"
			else:
				# we serve the file on disk
				super().do_GET()

if __name__ == '__main__':
	import os, pwd, grp

	server_addr = ('', 80)
	server = ThreadingHTTPServer(server_addr, Handler)

	os.chroot(".")
	os.setgid(grp.getgrnam("_www").gr_gid)
	os.setuid(pwd.getpwnam("_www").pw_uid)

	try:
		server.serve_forever()
	except KeyboardInterrupt:
		print()
		exit()
