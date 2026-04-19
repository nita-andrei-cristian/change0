mkdir debug
cd debug
clear
cmake -DCMAKE_BUILD_TYPE=Debug -DGGML_CUDA=ON -G Ninja ..

if [ -f ./.env ]; then
	echo "Setting ./.env"
	set -a
	source .env
	set +a
fi

cd ..
cmake --build debug --config Debug -j

cd debug

# This helps clang detect the file
mv compile_commands.json ../compile_commands.json

#./neuralnetwork
