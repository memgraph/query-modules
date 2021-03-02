import logging
from typing import Dict, Any, Optional, List
from telco.graph import Graph
from telco.components.population import Population
from telco.components.individual import Individual
from telco.components.chain_chunk import ChainChunk
from telco.components.chain_population import ChainPopulation
from telco.framework.parameters_utils import param_value
from telco.utils.validation import validate


logger = logging.getLogger('telco')


@validate("population_size", "no_of_colors")
def generate_individuals(
        graph: Graph,
        parameters: Dict[str, Any] = None) -> List[Individual]:
    """Creates a list of individuals in which some individuals are the result of the given algorithms.
    If more algorithms are given than the population size, then the excess is ignored."""

    population_size = param_value(graph, parameters, "population_size")
    no_of_colors = param_value(graph, parameters, "no_of_colors")
    algorithms = param_value(graph, parameters, "algorithms")

    individuals = []
    if algorithms is not None:
        for algorithm in algorithms:
            indv = algorithm.run(graph, parameters)
            if indv is None:
                logger.error('Population creation has not succeeded.')
                return None
            if len(individuals) < population_size:
                individuals.append(indv)

    individuals.extend([Individual(no_of_colors, graph) for _ in range(population_size - len(individuals))])
    return individuals


@validate("population_size", "no_of_chunks")
def create(
        graph: Graph,
        parameters: Dict[str, Any] = None) -> Optional[Population]:
    """Returns a list of no_of_chunks populations that have an equal number of individuals."""

    population_size = param_value(graph, parameters, "population_size")
    no_of_chunks = param_value(graph, parameters, "no_of_chunks")

    individuals = generate_individuals(graph, parameters)
    populations = []
    if no_of_chunks == 1:
        populations.append(ChainPopulation(graph, individuals))
        return populations

    chunks = _list_chunks(individuals, population_size, no_of_chunks)
    for i, chunk in enumerate(chunks):
        prev_chunk = i - 1 if i - 1 > 0 else no_of_chunks - 1
        next_chunk = i + 1 if i + 1 < no_of_chunks else 0
        populations.append(ChainChunk(graph, chunk, chunks[prev_chunk][-1], chunks[next_chunk][0]))
    return populations


def _list_chunks(
        individuals: List[Individual],
        population_size: int,
        no_of_chunks: int) -> List[List[Individual]]:
    """Splits a list into equal parts."""
    k, m = divmod(population_size, no_of_chunks)
    chunks = [list(individuals[i * k + min(i, m):(i + 1) * k + min(i + 1, m)]) for i in range(no_of_chunks)]
    return chunks
