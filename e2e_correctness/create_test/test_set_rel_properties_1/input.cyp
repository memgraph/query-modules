CREATE (idora:Person {name: "Idora", id:0}) CREATE (ivan:Person {name: "Ivan", id:1}) CREATE (matija:Person {name: "Matija", id:2}) MERGE (ivan)-[:FRIENDS {since: 'forever'}]-(idora) MERGE (ivan)-[:FRIENDS {since: 'forever and ever'}]-(matija);