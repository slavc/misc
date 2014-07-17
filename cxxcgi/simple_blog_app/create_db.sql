-- A script for Sqlite3 which creates the
-- neccessery tables.

create table categories (
    id integer primary key autoincrement,
    category text not null,
    description text,
    appearance_sequence integer not null
);

create table tags (
    id integer primary key autoincrement,
    tag text not null
);

create table posts (
    id integer primary key autoincrement,
    ctime datetime not null default CURRENT_DATETIME,
    mtime datetime not null default CURRENT_DATETIME,
    title text not null,
    content text not null
);

create table CategoriesPosts (
    category_id integer,
    post_id integer,
    foreign key (category_id) references categories (id),
    foreign key (post_id) references posts (id)
);

create table TagsPosts (
    tag_id integer, 
    post_id integer,
    foreign key (tag_id) references tags (id),
    foreign key (post_id) references posts (id)
);

