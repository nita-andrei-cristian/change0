# CHANGE

Welcome to the change project, so far the application decomposes user inputs into nodes, forming a cluster of nodes called User Identity.
See below a brief overview:

## Concepts overview
### The neuroengine (NE)
The Neuroengine is responsible for managing a User Idenitity, which is obviously akin to a naive neural structure.
#### Deep Search (DS)
The neuroengine supports Deep Search (Or Deep Research) to perform a task tailored to a user's identity
#### Decomposition system
This is the system responsible for input-processing and spliting human readable text into nodes.
### Server
The client application has a `server` feature, this allows Neuro Engine (And DS) data to be exposed via HTTP protocol. (Read instalation guide for running.)

## Future plans
- Make OpenAI optional, so you can host your own AI
- Goal implmenetation, where you can customize a dynamic real-life goal based on a user's identity. The goal dinamically syncs with the user context.
- Reintroduce mocks
- Clean the terminal client
- Much more

## To consider
- Testing the project requires an Open AI API key, you can switch manually to another system if you wish
- The project is 1. Work in progress, 2. Meant to be demo. 
- This is not conisdered safe by any means

# Instalation Guide
- You need linux, mac os or any other unix-like OS.
- You need Open SSL package and a valid Open AI key.
- Other common packages like cmake, gcc compiler are required.
- Optional but recommended : The http-server npm package

### Step 1
Clone this directory
```bash
git clone https://github.com/nita-andrei-cristian/change0.git
```

### Step 2
Setup an OpenAi key (Or any other key if you use a custom host)
```bash
export OPENAI_API_KEY=my_secret_key
```

### Step 3
Run build.sh
```bash
./build.sh
```

### Step 4
Now you will see terminal Cli interface.
You will most likely only care about the server:
- press `s` to start one. (Port 8085)
- make sure to not close the terminal

PS : If you want to change the port, modify config.h file and graph.html. Swap 8085 with another port

### Step 5
From another terminal:
- enter `change0/js` directory 
- run `http-server` (or any other command to run a server)
- open the received http url in a browser

### Step 6
- Use the app, but be patient because OpenAI requests are slow...
