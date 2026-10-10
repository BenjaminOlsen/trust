0: clone repo (git --recurse-submodules --shallow-submodules <url>)
 or if you already git cloned, and have an empty llvm path:
git submodule update --init --recursive --depth 1

----------------------------------------------------------
build docker image: 

  $ docker build -t clang-bootstrap .

-------------------------------------------
make a copy of llvm and check out the 23.1.2 source 
(example using git worktree:)

git -C llvm-project worktree add --detach \
  $PWD/llvm-project-23.1.2 \
  8b72d3bca3


----------------------------------------------------------
run the container, from the trust/ directory;

docker run -it \
  --name clang-edit \
  --hostname trust-clang \
  --mount type=volume,source=llvm-bootstrap,target=/work \
  --mount "type=bind,source=$PWD,target=/project" \
  --mount "type=bind,source=$PWD/llvm-project,target=/work/llvm-project" \
  --mount "type=bind,source=$PWD/llvm-project-23.1.2,target=/work/llvm-project-23.1.2,readonly" \
  clang-bootstrap


this 1. creates (or reuses if already existing) a docker managed volume named 'llvm-bootstrap' (name it whatever you want) in the host (wherever docker makes those things, depends on the os); and mounts it at '/work' inside the container.
2. binds $PWD in the host to /project in the container
3. binds $PWD/llvm-project to /work/llvm-project in the container
4. binds $PWD/llvm-project-23.1.2 to /work/llvm-project-23.1.2 as a READ ONLY volume in the the container


So inside the container:
/work/llvm-project          modified source
/work/llvm-project-23.1.2   clean, read only source
/project                    complete trust workspace
/work                       persistent build volume


dontt use --rm: keep the named container so you can reopen it later.
If clang-edit containers already exist, use 
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

------ next, build the clean clang source with the stage 2 binary:

cmake -S /work/llvm-project-23.1.2/llvm \
  -B /work/stage3 \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/work/stage2/bin/clang \
  -DCMAKE_CXX_COMPILER=/work/stage2/bin/clang++ \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_TARGETS_TO_BUILD=Native \
  -DLLVM_PARALLEL_LINK_JOBS=1

cmake --build /work/stage3 -j <parallel job cnt>

  docker start -ai clang-edit

  cmake --build /work/stage1 -j <parallel job cnt>
  cmake --build /work/stage2 -j <parallel job cnt>


To leave a build running and detach :
    Ctrl+P, Ctrl+Q.
Reconnect with docker attach clang-edit 


--------------------------------------
after stage 2, clang is at /work/stage2/bin/clang:
test compile hello.c from project dir:

  /work/stage2/bin/clang --version
  cd /project
  clang -std=c17 -Wall -Wextra hello.c -o hello
  ./hello


(LLVM bootstrap documentation: https://llvm.org/docs/AdvancedBuilds.html)

In LLVM's libc:

rand() is declared in llvm-project/libc/src/stdlib/rand.h, implemented in llvm-project/libc/src/stdlib/rand.cpp: `LLVM_LIBC_FUNCTION(int, rand, (void))`

cmake -G Ninja \
  -S llvm-project-23.1.2/runtimes \
  -B build-libc-23 \
  -DLLVM_ENABLE_RUNTIMES=libc \
  -DLLVM_LIBC_FULL_BUILD=OFF \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build build-libc-23 --target libc

clang hello.c /path/to/libllvmlibc.a -o hello

if you’re testing functions the compiler might optimize away, such as `strlen()`, add `-fno-builtin`

clang -fno-builtin hello.c /path/to/libllvmlibc.a -o hello

