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

### `run`

Build and run the application.

```bash
blaze run [flags]
```

*   **Description**: Compiles the C++ project using CMake and executes the binary.
*   **Flags**:
    *   `-w, --watch`: Enables **Hot Reload**. Watches `src/` and `include/` for changes, automatically recompiling and restarting the server.
    *   `-r, --release`: Builds in Release mode (`-O3`) for maximum performance.

**Example**:
```bash
# Development mode with hot-reload
blaze run --watch

# Production build
blaze run --release
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
    *   `frontend`: launches the frontend wizard to add a Vite-based frontend to an existing backend project.

**Example**:
```bash
blaze add postgres
blaze add test
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
