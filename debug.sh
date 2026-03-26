mkdir debug
cd debug
clear
cmake -DCMAKE_BUILD_TYPE=Debug -DGGML_CUDA=ON -G Ninja ..

cd ..
cmake --build debug --config Debug -j

cd debug

# This helps clang detect the file
mv compile_commands.json ../compile_commands.json

#./neuralnetwork
