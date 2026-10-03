FROM ubuntu:24.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential ca-certificates cmake git ninja-build python3 \
    && rm -rf /var/lib/apt/lists/*

# The interactive builds in readme.txt create this compiler in /work.
ENV PATH="/work/stage2/bin:${PATH}"
RUN echo "PS1='\u@\h:\w\$ '" >> /root/.bashrc
WORKDIR /work
CMD ["bash"]
