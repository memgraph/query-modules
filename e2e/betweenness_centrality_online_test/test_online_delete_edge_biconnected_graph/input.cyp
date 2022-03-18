setup: |-        
        MERGE (a: Node {id: 0}) MERGE (b: Node {id: 1}) CREATE (a)-[:RELATION]->(b);
        MERGE (a: Node {id: 0}) MERGE (b: Node {id: 3}) CREATE (a)-[:RELATION]->(b);
        MERGE (a: Node {id: 1}) MERGE (b: Node {id: 2}) CREATE (a)-[:RELATION]->(b);
        MERGE (a: Node {id: 2}) MERGE (b: Node {id: 3}) CREATE (a)-[:RELATION]->(b);
        MERGE (a: Node {id: 2}) MERGE (b: Node {id: 4}) CREATE (a)-[:RELATION]->(b);
        MERGE (a: Node {id: 3}) MERGE (b: Node {id: 5}) CREATE (a)-[:RELATION]->(b);
        MERGE (a: Node {id: 3}) MERGE (b: Node {id: 7}) CREATE (a)-[:RELATION]->(b);
        MERGE (a: Node {id: 4}) MERGE (b: Node {id: 7}) CREATE (a)-[:RELATION]->(b);
        MERGE (a: Node {id: 5}) MERGE (b: Node {id: 6}) CREATE (a)-[:RELATION]->(b);
        MERGE (a: Node {id: 6}) MERGE (b: Node {id: 7}) CREATE (a)-[:RELATION]->(b);
        CALL betweenness_centrality_online.set() YIELD *;
        CREATE TRIGGER test_delete_edge BEFORE COMMIT EXECUTE CALL betweenness_centrality_online.update(createdVertices, createdEdges, deletedVertices, deletedEdges) YIELD *;

queries:
    - |-        
        MATCH (a: Node {id: 3})-[r:RELATION]->(b: Node {id: 7}) DELETE r;


cleanup: |-
    DROP TRIGGER test_delete_edge;
    MATCH (n: Node) DETACH DELETE n;
    CALL mg.load('betweenness_centrality_online') YIELD *;
