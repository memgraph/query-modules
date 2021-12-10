<h1 align="center">
  <br>
  <a href="https://github.com/memgraph/mage"> <img src="https://github.com/memgraph/mage/blob/main/img/wizard.png?raw=true" alt="MAGE" width=20%></a>
  <br>
  MAGE
  <br>
</h1>

<p align="center">
    <a href="https://github.com/memgraph/mage/actions" alt="Actions">
        <img src="https://img.shields.io/github/workflow/status/memgraph/mage/Build%20and%20Test?label=build%20and%20test&logo=github"/>
    </a>
    <a href="https://github.com/memgraph/mage/blob/main/LICENSE" alt="Licence">
        <img src="https://img.shields.io/github/license/memgraph/mage" />
    </a>
    <a href="https://docs.memgraph.com/mage/" alt="Documentation">
        <img src="https://img.shields.io/badge/documentation-MAGE-orange" />
    </a>
    <a href="https://hub.docker.com/r/memgraph/memgraph-mage" alt="Documentation">
        <img src="https://img.shields.io/badge/image-Docker-2496ED?logo=docker" />
    </a>
    <a href="https://github.com/memgraph/mage" alt="Languages">
        <img src="https://img.shields.io/github/languages/count/memgraph/mage" />
    </a>
    <a href="https://github.com/memgraph/mage/stargazers" alt="Stargazers">
        <img src="https://img.shields.io/github/stars/memgraph/mage?style=social" />
    </a>
</p>

## Memgraph Advanced Graph Extensions :crystal_ball:

This open-source repository contains all available user-defined graph analytics modules and procedures that extend the Cypher query language, written by the team behind Memgraph and its users. You can find and contribute implementations of various algorithms in multiple programming languages, all runnable inside Memgraph. This project aims to give everyone the tools they need to tackle the most challenging graph problems.

### Introduction to query modules with MAGE
Memgraph introduces the concept of [query modules](https://docs.memgraph.com/memgraph/reference-guide/query-modules/),
user-defined procedures that extend the Cypher query language. These procedures are grouped into modules that can be loaded into Memgraph. How to run them can be seen on their official
[documentation](https://docs.memgraph.com/mage/usage/loading-modules).
When started, Memgraph will automatically attempt to load the query modules from all `*.so` and `*.py` files it finds in the default directory defined with flag
[--query-modules-directory](https://docs.memgraph.com/memgraph/reference-guide/configuration/).

### Further reading
If you want more info about MAGE, check out the official [MAGE Documentation](https://docs.memgraph.com/mage/).

### Algorithm proposition
Furthermore, if you have an **algorithm proposition**, please fill in the survey on [**mage.memgraph.com**](https://mage.memgraph.com/).

### Community
Make sure to check out Memgraph community, and join us on a survey of streaming a graph algorithms! Drop us a message on the channels below:

- :robot: [**Discord**](https://discord.gg/memgraph)
- :busts_in_silhouette: [**Discourse forum**](https://discourse.memgraph.com/)
- :octocat: [**Memgraph GitHub**](https://github.com/memgraph)
- :bird: [**Twitter**](https://twitter.com/memgraphdb)
- :movie_camera: [**YouTube**](https://www.youtube.com/channel/UCZ3HOJvHGxtQ_JHxOselBYg)
-
<a href="https://twitter.com/intent/follow?screen_name=memgraphmage">
    <img src="https://img.shields.io/badge/@memgraphmage-1DA1F2?style=flat-square&logo=twitter&logoColor=white" alt="Follow @memgraphmage"/>
  </a>
<a href="https://discourse.memgraph.com/">
    <img src="https://img.shields.io/badge/Discourse_forum-000000?style=flat-square&logo=discourse&logoColor=white" alt="Discourse forum"/>
</a>
<a href="https://memgr.ph/join-discord">
    <img src="https://img.shields.io/badge/Discord-7289DA?style=flat-square&logo=discord&logoColor=white" alt="Discord"/>
</a>
<a href="https://github.com/memgraph">
    <img src="https://img.shields.io/badge/Memgraph_GitHub-181717?style=flat-square&logo=github&logoColor=white" alt="Memgraph Github"/>
</a>
<a href="https://www.youtube.com/channel/UCZ3HOJvHGxtQ_JHxOselBYg">
    <img src="https://img.shields.io/badge/YouTube-FF0000?style=flat-square&logo=youtube&logoColor=white" alt="Memgraph YouTube"/>
</a>

## Overview
- [Memgraph compatibility](#memgraph-compatibility)
  - [How to install?](#how-to-install)
  - [Further steps - Installation](#further-steps---installation)
    - [1. Installing MAGE with Docker](#1-installing-mage-with-docker)
      - [a) Install MAGE from Docker Hub](#a-install-mage-from-docker-hub)
      - [b) Install MAGE with Docker build of repository](#b-install-mage-with-docker-build-of-repository)
    - [2. Installing MAGE on Linux distro from source](#2-installing-mage-on-linux-distro-from-source)
      - [Prerequisites](#prerequisites)
  - [Running the MAGE](#running-the-mage)
  - [MAGE Spells](#mage-spells)
  - [Advanced configuration](#advanced-configuration)
  - [Testing the MAGE](#testing-the-mage)
  - [Contributing](#contributing)
  - [Code of Conduct](#code-of-conduct)
  - [Feedback](#feedback)

# Memgraph compatibility
With changes in Memgraph API, MAGE started to track version numbers. Check out the table below which will tell you compatibility of MAGE with Memgraph versions.
| MAGE version | Memgraph version  |
| ------------ | ----------------- |
| >= 1.0       | >= 2.0.0          |
| ^0           | >= 1.4.0 <= 1.6.1 |
## How to install?
There are two options to install MAGE. With [Docker installation](#1-installing-mage-with-docker) you only need Docker.
[To build from source](#2-installing-mage-locally-with-linux-package-of-memgraph)
you will need **Python3**, **Make**, **CMake**, **Clang**, **UUID** and **Rust**. Installation with Docker is easier for quick installation
and smaller development.

## Further steps - Installation
After installation part, you will be ready to query Memgraph and use **MAGE** modules. Make sure to have one of [the querying
platform](https://memgraph.com/docs/memgraph/connect-to-memgraph/).

### 1. Installing MAGE with Docker

#### a) Install MAGE from Docker Hub

> Note: Here you don't need to download Github Memgraph/MAGE repository.

**1.** This command downloads image from Docker Hub and runs Memgraph with **MAGE** algorithms:
```
docker run -p 7687:7687 memgraph/memgraph-mage
```


#### b) Install MAGE with Docker build of repository

**0.** Make sure that you have cloned MAGE Github repository and positioned yourself inside repo in terminal.
To clone Github repository and position yourself inside `mage` folder, do the following in terminal:

```bash
git clone https://github.com/memgraph/mage.git && cd mage
```

**1.** To build **MAGE** image run the following command:
```
docker build  -t memgraph-mage .
```
This will build any new algorithm added to MAGE, and load it inside Memgraph.

**2.** Start image with the following command and enjoy your own **MAGE**:
```
docker run --rm -p 7687:7687 --name mage memgraph-mage
```


**NOTE**: if you made any new changes while **MAGE** Docker container is running, you need to stop it and rebuild whole image,
or you can copy mage folder inside **MAGE** docker container and just do the rebuild.
Jump to [build MAGE query modules with Docker](#building-and-loading-modules-inside-memgraph-with-docker)


### 2. Installing MAGE on Linux distro from source
> Note: This step is more suitable for local development.

#### Prerequisites
* Linux based Memgraph package you can download [here](https://memgraph.com/download). We offer Ubuntu, Debian, Centos based Memgraph
packages. To install, proceed to the following [site](https://memgraph.com/docs/memgraph/installation).
* To build and install MAGE query modules you will need: **Python3**, **Make**, **CMake**, **Clang**, **UUID** and **Rust**.

Since Memgraph needs to load MAGE's modules, there is the `setup` script to help you.


Run the `build` command of the `setup` script. It will generate a `mage/dist` directory with all the `*.so` and `*.py` files.
Flag `-p (--path)`  represents where will contents of `mage/dist` directory be copied. You need to copy it to
`/usr/lib/memgraph/query_modules` directory, because that's where Memgraph is looking for query modules by
[default](https://docs.memgraph.com/memgraph/reference-guide/configuration/).

```
python3 setup build -p /usr/lib/memgraph/query_modules
```
> Note that query modules are loaded into Memgraph on startup so if your instance was already running you will need to
> execute the following query inside one of [querying platforms](https://docs.memgraph.com/memgraph/connect-to-memgraph) to load them:

```
CALL mg.load_all();
```

## Running the MAGE
If we have a graph that is broken into multiple components (left image), simple call this MAGE spell to check out which node is in which components (right image) →

```
// Create graph as on image below

CALL weakly_connected_components.get() YIELD node, component_id
RETURN node, component_id;
```

|       Graph input        |        MAGE output        |
| :----------------------: | :-----------------------: |
| ![](img/graph_input.png) | ![](img/graph_output.png) |


## MAGE Spells

| Algorithms                                                                                         | Lang   | Description                                                                                                                                                                                                                       |
| -------------------------------------------------------------------------------------------------- | ------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [betweenness_centrality](cpp/betweenness_centrality_module/betweenness_centrality_module.cpp)      | C++    | The betweenness centrality of a node is defined as the sum of the of all-pairs shortest paths that pass through the node divided by the number of all-pairs shortest paths in the graph. The algorithm has O(nm) time complexity. |
| [biconnected_components](cpp/biconnected_components_module/biconnected_components_module.cpp)      | C++    | Algorithm for calculating maximal biconnected subgraph. A biconnected subgraph is a subgraph with a property that if any vertex were to be removed, the graph will remain connected.                                              |
| [bipartite_matching](cpp/bipartite_matching_module/bipartite_matching_module.cpp)                  | C++    | Algorithm for calculating maximum bipartite matching, where matching is a set of nodes chosen in such a way that no two edges share an endpoint.                                                                                  |
| [bridges](cpp/bridges_module/bridges_module.cpp)                                                   | C++    | A bridge is an edge, which when deleted, increases the number of connected components. The goal of this algorithm is to detect edges that are bridges in a graph.                                                                 |
| [community_detection_online](cpp/community_detection_module/community_detection_online_module.cpp) | C++    | A dynamic community detection algorithm suitable for large-scale graphs based upon label propagation. Runs in O(m) time and has O(mn) space complexity.                                                                           |
| [cycles](cpp/cycles_module/cycles_module.cpp)                                                      | C++    | Algorithm for detecting cycles on graphs                                                                                                                                                                                          |
| [distance_calculator](python/distance_calculator.py)                                               | Python | Module for finding the geographical distance between two points defined with 'lng' and 'lat' coordinates.                                                                                                                         |
| [graph_analyzer](python/graph_analyzer.py)                                                         | Python | This Graph Analyzer query module offers insights about the stored graph or a subgraph.                                                                                                                                            |
| [graph_coloring](python/graph_coloring.py)                                                         | Python | Algorithm for assigning labels to the graph elements subject to certain constraints. In this form, it is a way of coloring the graph vertices such that no two adjacent vertices are of the same color.                           |
| [node2vec](python/node2vec.py)                                                                     | Python | An algorithm for calculating node embeddings on static graph.                                                                                                                                                                     |
| [node2vec_online](python/node2vec_online.py)                                                       | Python | An algorithm for calculating node embeddings as new edges arrive                                                                                                                                                                  |
| [node_similarity](python/node_similarity.py)                                                       | Python | A module that contains similarity measures for calculating the similarity between two nodes.                                                                                                                                      |
| [nxalg](python/nxalg.py)                                                                           | Python | A module that provides NetworkX integration with Memgraph and implements many NetworkX algorithms                                                                                                                                 |
| [pagerank](cpp/pagerank_module/pagerank_module.cpp)                                                | C++    | Algorithm that yields the influence measurement based on the recursive information about the connected nodes influence                                                                                                            |
| [pagerank_online](cpp/pagerank_module/pagerank_online_module.cpp)                                  | C++    | A dynamic algorithm made for calculating PageRank in a graph streaming scenario.                                                                                                                                                  |
| [rust_example](cpp/pagerank_module/pagerank_online_module.cpp)                                     | Rust   | Example of a basic module with input parameters forwarding, made in Rust.                                                                                                                                                         |
| [set_cover](python/set_cover.py)                                                                   | Python | The algorithm for finding minimum cost subcollection of sets that covers all elements of a universe.                                                                                                                              |
| [tsp](python/tsp.py)                                                                               | Python | An algorithm for finding the shortest possible route that visits each vertex exactly once.                                                                                                                                        |
| [union_find](python/union_find.py)                                                                 | Python | A module with an algorithm that enables the user to check whether the given nodes belong to the same connected component.                                                                                                         |
| [uuid_generator](cpp/uuid_module/uuid_module.cpp)                                                  | C++    | A module that generates a new universally unique identifier (UUID).                                                                                                                                                               |
| [vrp](python/vrp.py)                                                                               | Python | Algorithm for finding the shortest route possible between the central depot and places to be visited. The algorithm can be solved with multiple vehicles that represent a visiting fleet.                                         |
| [weakly_connected_components](cpp/connectivity_module/connectivity_module.cpp)                     | C++    | A module that finds weakly connected components in a graph.                                                                                                                                                                       |

## Advanced configuration
- [Advanced query module directories setup](https://memgraph.com/docs/mage/installation#advanced-configuration) (under `Build from source on Linux`)
- [Developing MAGE with Docker](https://memgraph.com/docs/mage/installation#developing-mage-with-docker) (under `Docker build`)

## Testing the MAGE
To test that everything is built, loaded, and working correctly, a python script can be run. Make sure that the Memgraph instance with **MAGE** is up and running.
```
# Running unit tests for C++ and Python
python3 test_unit

# Running end-to-end tests
python3 test_e2e
```

Furthermore, to test only specific end-to-end tests, you can add argument `-k` with substring referring to the algorithm that needs to be tested. To test a module named `<query_module>`, you would have to run `python3 test_e2e -k <query_module>` where `<query_module>` is the name of the specific module you want to test.
```
# Running specific end-to-end tests
python3 test_e2e -k weakly_connected_components
```
## Contributing

We encourage everyone to contribute with their own algorithm implementations and ideas. If you want to contribute or report a bug, please take a look at the [contributions guide](CONTRIBUTING.md).

## Code of Conduct

Everyone participating in this project is governed by the [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to <tech@memgraph.com>.

## Feedback
Your feedback is always welcome and valuable to us. Please don't hesitate to post on our [Community Forum](https://discourse.memgraph.com/).
