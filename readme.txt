0: clone repo (git --recurse-submodules --shallow-submodules <url>)
 or if you already git cloned, and have an empty llvm path:
git submodule update --init --recursive --depth 1

----------------------------------------------------------
build docker image: 

  $ docker build -t clang-bootstrap .

----------------------------------------------------------
run the container:

  $ docker run -it --name clang-work \
    --mount type=volume,source=llvm-bootstrap,target=/work \
    --mount "type=bind,source=$PWD,target=/project" \
    clang-bootstrap

---- to mount the llvm-project dir inside the container, run the following from the dir containing it:
  $ docker run -it --name clang-edit \
    --mount type=volume,source=llvm-bootstrap,target=/work \
    --mount "type=bind,source=$PWD,target=/project" \
    --mount "type=bind,source=$PWD/llvm-project,target=/work/llvm-project" \
    clang-bootstrap

this runs the 1. creates (or reuses if already existing) a docker managed volume named 'llvm-bootstrap' (name it whatever you want) in the host (wherever docker makes those things, depends on the os); and mounts it at '/work' inside the container.
2. binds $PWD in the host to /project in the container
3. binds $PWD/llvm-project to /work/llvm-project in the container

dontt use --rm: keep the named container so you can reopen it later.
If clang-work or clang-edit containers already exist, use 
    docker start -ai clang-work
    docker start -ai clang-edit

-----------------------------------------------------------------
Configure stage 1 (once), then build it with GCC/G++:

  cmake -S /work/llvm-project/llvm -B /work/stage1 -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=/usr/bin/gcc \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DLLVM_ENABLE_PROJECTS=clang \
    -DLLVM_TARGETS_TO_BUILD=Native \
    -DLLVM_PARALLEL_LINK_JOBS=1

  cmake --build /work/stage1 -j <parallel job cnt>

3. After stage 1 finishes configure and build stage 2:

  cmake -S /work/llvm-project/llvm -B /work/stage2 -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=/work/stage1/bin/clang \
    -DCMAKE_CXX_COMPILER=/work/stage1/bin/clang++ \
    -DLLVM_ENABLE_PROJECTS=clang \
    -DLLVM_TARGETS_TO_BUILD=Native \
    -DLLVM_PARALLEL_LINK_JOBS=1

  cmake --build /work/stage2 -j <parallel job cnt>

  docker start -ai clang-work

  cmake --build /work/stage1 -j <parallel job cnt>
  cmake --build /work/stage2 -j <parallel job cnt>


To leave a build running and detach :
    Ctrl+P, Ctrl+Q.
Reconnect with docker attach clang-work.


--------------------------------------
after stage 2, clang is at /work/stage2/bin/clang:
test compile hello.c from project dir:

  /work/stage2/bin/clang --version
  cd /project
  clang -std=c17 -Wall -Wextra hello.c -o hello
  ./hello


(LLVM bootstrap documentation: https://llvm.org/docs/AdvancedBuilds.html)
