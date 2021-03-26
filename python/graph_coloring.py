import mgp
from collections import defaultdict
from typing import List, Optional
from mage.graph_coloring_module import Graph
from mage.graph_coloring_module import QA
from mage.graph_coloring_module import ConflictError
from mage.graph_coloring_module import SimpleMutation
from mage.graph_coloring_module import MultipleMutation
from mage.graph_coloring_module import LDO
from mage.graph_coloring_module import SDO


@mgp.read_proc
def color_graph(
    context: mgp.ProcCtx, parameters: mgp.Map
) -> mgp.Record(node=str, color=str):
    g = _convert_to_graph(context)
    sol = _run_algorithm(g, parameters)
    return [
        mgp.Record(node=str(g.label(node)), color=str(color))
        for node, color in enumerate(sol)
    ]


@mgp.read_proc
def color_subgraph(
    context: mgp.ProcCtx,
    vertices: mgp.List[mgp.Vertex],
    edges: mgp.List[mgp.Edge],
    parameters: mgp.Map,
) -> mgp.Record(node=str, color=str):
    g = _convert_to_subgraph(context, vertices, edges)
    sol = _run_algorithm(g, parameters)
    return [
        mgp.Record(node=str(g.label(node)), color=str(color))
        for node, color in enumerate(sol)
    ]


def _run_algorithm(graph: Graph, parameters) -> List[int]:
    alg = QA()
    _default_parameters = {
        "no_of_processes": 1,
        "no_of_chunks": 1,
        "communication_delay": 10,
        "max_iterations": 10,
        "population_size": 7,
        "no_of_colors": 3,
        "max_steps": 25,
        "temperature": 0.035,
        "mutation": SimpleMutation(),
        "error": ConflictError(),
        "alpha": 0.1,
        "beta": 0.001,
        "algorithms": [LDO(), SDO()],
        "logging_delay": 5,
        "convergence_tolerance": 500,
        "convergence_probability": 0.5,
        "max_attempts_tunneling": 10,
        "neigh_mutation_probability": 0.035,
        "mutation_tunneling": MultipleMutation(),
        "multiple_mutation_no_of_nodes": 5,
        "random_mutation_probability": 0.1,
        "random_mutation_probability_2": 0.5,
    }
    sol = alg.run(graph, _default_parameters)
    return sol.chromosome


def _convert_to_graph(context: mgp.ProcCtx) -> Graph:
    nodes = []
    adj_list = defaultdict(list)

    for v in context.graph.vertices:
        context.check_must_abort()
        nodes.append(v.id)

    for v in context.graph.vertices:
        context.check_must_abort()
        for e in v.out_edges:
            weight = e.properties["weight"]
            adj_list[e.from_vertex.id].append((e.to_vertex.id, weight))
            adj_list[e.to_vertex.id].append((e.from_vertex.id, weight))

    return Graph(nodes, adj_list)


def _convert_to_subgraph(
    context: mgp.ProcCtx, vertices: mgp.List[mgp.Vertex], edges: mgp.List[mgp.Edge]
) -> Optional[Graph]:

    nodes = []
    adj_list = defaultdict(list)

    for v in vertices:
        context.check_must_abort()
        nodes.append(v.id)

    for e in edges:
        context.check_must_abort()
        weight = e.properties["weight"]
        adj_list[e.from_vertex.id].append((e.to_vertex.id, weight))
        adj_list[e.to_vertex.id].append((e.from_vertex.id, weight))

    return Graph(nodes, adj_list)
