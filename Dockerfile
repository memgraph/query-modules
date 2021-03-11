FROM memgraph:1.3.0-community AS memgraph-mage

FROM memgraph-mage
USER root

RUN apt-get update && \
    apt-get --yes install rsync cmake clang

# Set mage as working directory
WORKDIR /mage
COPY . /mage

# Build all necessary modules
RUN /bin/bash build.sh
RUN cp -r /mage/dist/* /usr/lib/memgraph/query_modules/

# It's required to install python3 because auth module scripts are going to be
# written in python3.
USER root
RUN apt-get install -y python3-dev \
  && cd /usr/local/bin \
  && ln -s /usr/bin/python3 python \
  && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

USER memgraph
ENV LD_LIBRARY_PATH /usr/lib/memgraph/query_modules

# Memgraph listens for Bolt Protocol on this port by default.
EXPOSE 7687
# Snapshots and logging volumes
VOLUME /var/log/memgraph
VOLUME /var/lib/memgraph
VOLUME /etc/memgraph

USER memgraph
WORKDIR /usr/lib/memgraph/query_modules

ENTRYPOINT ["/usr/lib/memgraph/memgraph"]
CMD [""]