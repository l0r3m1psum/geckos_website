-- sqlite3 -bail -init $file

-- TODO: maybe add some cool pragma (like WAL)

.headers on
.mode column

pragma foreign_keys = on;
drop table if exists geckos;
drop view if exists geckos_catalog;

create table geckos (
	id       integer  primary key,
	morph    text     not null,
	sex      text     not null check(sex = 'M' or sex = 'F'),
	born     text     check (date(born) is not null),
	price    integer  not null check(price >= 0),
	father   integer,
	mother   integer,
	sellable integer  not null check(sellable = 1 or sellable = 0),
	-- picture blob not null,

	foreign key(father) references geckos(id)
		on delete set null
		on update cascade, -- when the id of the father changes we do it to
	foreign key(mother) references geckos(id)
		on delete set null
		on update cascade
);

-- TODO: maybe the picture could be stored as a blob, and served directy from
--       the database. In this way i could also  easly store a gallery foreach
--       gecko. http://example.com/pic?rowid=?
/*
create table gallery (
	rowid   integer primary key,
	id      integer not null,
	picture blob    not null,

	foreign key(id) references geckos(id)
		on delete cascade
		on update cascade
);
-- explain query plan select rowid from gallery where id = ?;
create index idx_gallery
	on gallery(id);
insert into gallery(picture) values (readfile('pic/1.jpg'));
*/

-- Mekes sure that when inserting a gecko with a known father it has the right
-- sex.
create trigger father_sex_insert
	before insert on geckos
	when new.father != null
begin
	select
		case
			when 'M' not in (select sex from geckos where id = new.father) then
				raise (abort, 'Invalid father sex')
		end;
end;

-- Mekes sure that when inserting a gecko with a known mother it has the right
-- sex.
create trigger mother_sex_insert
	before insert on geckos
	when new.mother != null
begin
	select
		case
			when 'F' not in (select sex from geckos where id = new.mother) then
				raise (abort, 'Invalid mother sex')
		end;
end;

-- Makes sure that that when changing the father of a gecko it is still a male.
create trigger father_change
	before update on geckos
	when new.father != old.father
begin
	select
		case
			when 'F' in (select sex from geckos where id = new.father) then
				raise (abort, 'Invalid father change')
		end;
end;

-- Makes sure that that when changing the mother of a gecko it is still a female.
create trigger mother_change
	before update on geckos
	when new.mother != old.mother
begin
	select
		case
			when 'M' in (select sex from geckos where id = new.mother) then
				raise (abort, 'Invalid mother change')
		end;
end;

-- Makes sure that when changing the sex of a gecko it doesn't have sons to
-- avoid parent with the wrong sex. Basically you can change the sex of a gecko
-- if and only if it does not have sons. This makes sex changes harder...
create trigger parent_sex_update
	before update on geckos
	when new.sex != old.sex
begin
	select
		case
			-- at this point the sex must be wrong because it was changed
			when exists(
				select * from geckos where father = new.id or mother = new.id
				) then
				raise (abort, 'Invalid sex change')
		end;
end;

-- TODO: maybe split this in two different triggers
-- Makes sure that the son is born after its parent, this has the side effect of
-- making sure that a son is not a parent for itself.
create trigger born_after_parents
	before insert on geckos
	when new.father != null or new.mother != null
begin
	select
		case
			when exists(
				select *
				from geckos
				where id = new.father or id = new.mother
					and strftime('%s', new.born) > strftime('%s', born)
				) then
				raise (abort, 'Invalid date of birth')
		end;
end;

-- Make sure that when a birth date is updated it is still after the one of the
-- parents, and if it has sons they are still born after it.
create trigger born_change
	before update on geckos
	when new.born != old.born
begin
	select
		case
			-- if it has parents
			when exists(
				select *
				from geckos
				where id = new.father or id = new.mother
					and strftime('%s', new.born) > strftime('%s', born)
				) then
				raise (abort, 'Invalid date of birth for son')
			-- if it has sons
			when exists(
				select *
				from geckos
				where father = new.id or mother = new.id
					and strftime('%s', new.born) < strftime('%s', born)
				) then
				raise (abort, 'Invalid date of birth for parent')
		end;
end;

-- explain query plan select * from catalog order by date(born) desc;
-- NOTE: indexing like this (sellable, born) doesn't improve performances
create index idx_catalog
	on geckos(sellable);

-- To make the landing page code easier
create view catalog as
	select id, morph, sex, born, price, father, mother
	from geckos
	where sellable = 1;

-- TODO: creare trigger per inserire nel catalogo
-- TODO: rimpiazzare il delete su geckos per modificare lo stato di sellable da
--       1 a 0 (perché le informazioni sui gechi vanno tenuti anche dopo averli)
--       venduti siccome sono genitori di x) oppure eliminare proprio se
--       sellable è già 0.

-- Test code starts here -------------------------------------------------------

insert into geckos(morph, sex, born, price, father, mother, sellable)
	values
		-- ('blood',      'M', '2018-11-09', 100, null, null, 1),
		-- ('snow',       'F', '2019-01-15', 100, null, null, 0),
		-- ('normal',     'F', '2019-07-03', 100, null, null, 1),
		-- ('tangerine',  'M', '2019-07-28', 120, null, null, 1),
		-- ('eclipse',    'M', '2019-09-06', 50,  null, null, 0),
		-- ('albino',     'M', '2019-10-10', 150, 1,    2,    1),
		-- ('black night','F', '2020-05-19', 170, 1,    3,    1),
		-- ('albino2',    'M', '2021-10-11', 150, null, 2,    1);
		('super snow eclipse',             'F', '2018-01-01', 80, null, null, 0),
		('mack snow het tremper lavender', 'M', '2018-01-01', 50, null, null, 0),
		('mack snow het tremper eclipse',  'M', '2020-06-01', 60, 2, 1, 1),
		('mack snow het tremper eclipse',  'M', '2020-06-01', 60, 2, 1, 1),
		('emerine cross het rainwater poss tangerine ph eclipse', 'M', '2020-06-01', 60, null, null, 1); -- todo: genitori

-- update geckos
-- set father = 4
-- where id = 5;

-- -- TODO: verificare perché fallisce (forse centra l'ordine delle operazioni)
-- -- update geckos
-- -- set born = date(born, '+1 day');

-- update geckos
-- set father = 4
-- where id = 7;

-- select * from geckos;
-- select * from catalog;

.save geckos.db

-- this doesn't work...
.quit
