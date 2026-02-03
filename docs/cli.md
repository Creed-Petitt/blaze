# Blaze CLI Reference

The **Blaze CLI** is your primary tool for developing, building, and deploying Blaze applications. It handles project scaffolding, dependency management, hot-reloading, and Docker integration.

---

## Installation

If you haven't installed the CLI yet, run the install script:

```bash
curl -fsSL https://raw.githubusercontent.com/Creed-Petitt/blaze/main/install.sh | bash
```

Or build it from source (requires Go):

```bash
git clone https://github.com/Creed-Petitt/blaze
cd blaze/cli
go build -o blaze main.go
sudo mv blaze /usr/local/bin/
```

---

## Commands

### `init`

Initialize a new Blaze project.

```bash
blaze init <project_name> [flags]
```

*   **Description**: Creates a new directory with the project structure, `CMakeLists.txt`, `Dockerfile`, and a basic `main.cpp`.
*   **Interactive Mode**: Prompts you to select backend features like Database drivers (PostgreSQL, MySQL) and Testing (Catch2).
*   **Flags**:
    *   `-f, --fullstack`: Launches the frontend selection wizard (React, Vue, Svelte, Solid, Vanilla) to create a full-stack monorepo.

**Example**:
```bash
blaze init my-api
blaze init my-app --fullstack
```

---

### `dev`

Start the Fullstack Development Server.

```bash
blaze dev [flags]
```

*   **Description**: The ultimate command for local development. It launches your C++ Backend and (if present) your Vite Frontend simultaneously.
*   **Features**:
    *   **Auto-Install**: Automatically detects missing `node_modules` and runs `npm install`.
    *   **Dashboard**: Shows real-time status of your Backend, Frontend, and API Docs.
    *   **Unified Logs**: Manages output from both services in a clean interface.
*   **Flags**:
    *   `-w, --watch`: Enables **Hot Reload**. Watches `src/` and `include/` for changes, automatically recompiling and restarting the backend.
    *   `-r, --release`: Runs the backend in Release mode (`-O3`) for performance testing.

**Example**:
```bash
# Start in static mode (no hot-reload)
blaze dev

# Start with Hot Reload (Recommended)
blaze dev --watch
```

---

### `run`

Build and run the Backend (API only).

```bash
blaze run [flags]
```

*   **Description**: Compiles the C++ project using CMake and executes the binary. Ideal for API-only projects.
*   **Flags**:
    *   `-w, --watch`: Enables **Hot Reload**.
    *   `-r, --release`: Builds in Release mode.

**Example**:
```bash
blaze run -w
```

---

### `build`

Compile the project for deployment.

```bash
blaze build [flags]
```

*   **Description**: Compiles the project into a standalone binary located in `build/`.
*   **Flags**:
    *   `--prod`: **Production Bundle Mode**.
        *   Runs `npm install && npm run build` (if frontend exists).
        *   Moves built assets to the backend's `public/` folder.
        *   Compiles the C++ binary in **Release Mode**.
        *   Result: A single binary that serves your entire fullstack app.
    *   `-r, --release`: Force Release mode (default for `build` unless overridden).
    *   `-d, --debug`: Force Debug mode.

**Example**:
```bash
# Create a deployment-ready binary
blaze build --prod
```

---

### `add`

Add features or drivers to an existing project.

```bash
blaze add <feature>
```

*   **Subcommands**:
    *   `postgres`: Adds PostgreSQL driver and updates `CMakeLists.txt`.
    *   `mysql`: Adds MySQL/MariaDB driver and updates `CMakeLists.txt`.
    *   `test`: Adds a Catch2 testing suite and a `tests/` directory.

**Example**:
```bash
blaze add postgres
```

---

### `generate` (alias `g`)

Scaffold boilerplate code to save time.

```bash
blaze generate <type> <name>
```

*   **Subcommands**:
    *   `controller <name>`: Creates a Controller class in `src/controllers` and `include/controllers`.
        *   Example: `blaze g controller User` -> Creates `UserController`.
    *   `model <name>`: Creates a Model struct with `BLAZE_MODEL` macro in `include/models`.
        *   Example: `blaze g model Product` -> Creates `Product` struct.

---

### `test`

Run the project's test suite.

```bash
blaze test
```

*   **Description**: Compiles and runs the tests defined in the `tests/` directory using the Catch2 framework.
*   **Prerequisite**: You must have run `blaze add test` first.

---

### `docker`

Manage Docker containers for your application and services.

```bash
blaze docker <command>
```

*   **Description**: A wrapper around `docker` and `docker compose` to simplify local development.
*   **Subcommands**:
    *   `build`: Builds the Docker image for your application.
    *   `run`: Builds and runs the application container on port 8080.
    *   `psql`: Starts a PostgreSQL container (port 5432) in the background.
    *   `mysql`: Starts a MySQL container (port 3306) in the background.
    *   `redis`: Starts a Redis container (port 6379) in the background.
    *   `logs`: Tails the logs of all running background services.
    *   `stop`: Stops and removes all background services.

**Example Workflow**:
```bash
# Start a database
blaze docker psql

# Run your app in a container
blaze docker run

# View logs
blaze docker logs

# Cleanup
blaze docker stop
```
