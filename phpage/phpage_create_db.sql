/*
 * This script is not meant to be run manually!
 * Please use install_db.sh instead.
 */
--------------------------------------------------------------------------- 
CREATE USER $dbname WITH ENCRYPTED PASSWORD '$dbpasswd'; 
--------------------------------------------------------------------------- 
CREATE DATABASE $dbname
  WITH OWNER = $dbname
       ENCODING = 'UTF8'; 
---------------------------------------------------------------------------
\c $dbname
---------------------------------------------------------------------------
CREATE TABLE articles
(
  id serial NOT NULL,
  title character varying(1024),
  "text" text,
  page_id integer,
  ctime timestamp without time zone,
  mtime timestamp without time zone,
  CONSTRAINT articles_pkey PRIMARY KEY (id),
  CONSTRAINT articles_page_id_fkey FOREIGN KEY (page_id)
      REFERENCES pages (id) MATCH SIMPLE
      ON UPDATE NO ACTION ON DELETE NO ACTION
)
WITH (OIDS=FALSE);
--------------------------------------------------------------------------- 
CREATE TABLE banned_ips
(
  ip text NOT NULL,
  CONSTRAINT banned_ips_pkey PRIMARY KEY (ip)
)
WITH (OIDS=FALSE);
---------------------------------------------------------------------------
CREATE TABLE comments
(
  id serial NOT NULL,
  title character varying(1024),
  "text" text,
  "name" character varying(256),
  email character varying(256),
  website character varying(1024),
  article_id serial NOT NULL,
  ip character varying(256),
  ctime timestamp without time zone,
  CONSTRAINT comments_pkey PRIMARY KEY (id),
  CONSTRAINT comments_article_id_fkey FOREIGN KEY (article_id)
      REFERENCES articles (id) MATCH SIMPLE
      ON UPDATE NO ACTION ON DELETE NO ACTION
)
WITH (OIDS=FALSE);
---------------------------------------------------------------------------
CREATE TABLE pages
(
  id serial NOT NULL,
  title character varying(1024) NOT NULL,
  descr text,
  priority integer,
  CONSTRAINT pages_pkey PRIMARY KEY (id)
)
WITH (OIDS=FALSE);
---------------------------------------------------------------------------
CREATE TABLE tags
(
  article_id serial NOT NULL,
  tag text NOT NULL,
  CONSTRAINT tags_article_id_fkey FOREIGN KEY (article_id)
      REFERENCES articles (id) MATCH SIMPLE
      ON UPDATE NO ACTION ON DELETE NO ACTION,
  CONSTRAINT tags_article_id_key UNIQUE (article_id, tag)
)
WITH (OIDS=FALSE);
---------------------------------------------------------------------------
CREATE TABLE users
(
  "login" character varying(256) NOT NULL,
  "password" text NOT NULL,
  "class" "char" NOT NULL,
  CONSTRAINT users_pkey PRIMARY KEY (login)
)
WITH (OIDS=FALSE);

