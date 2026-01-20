# Blaze CLI Reference

The Blaze CLI (`blaze`) is your all-in-one tool for scaffolding, building, and managing high-performance C++ web applications.

## Global Usage

```bash
blaze [command]
```

## Commands

### `init`
Scaffolds a new Blaze project with a standardized directory structure.

```bash
blaze init <project-name> [--fullstack]
```
**Flags:**
*   `--fullstack`, `-f`: (Optional) Prompts you to select a frontend framework (React, Vue, Svelte, Solid, or Vanilla) and scaffolds a Vite-based project in the `/frontend` directory.

**Creates:**
*   `src/main.cpp`: Entry point.
*   `CMakeLists.txt`: Build configuration.
*   `Dockerfile`: Multi-stage production build.
*   `frontend/`: (If fullstack) Complete Vite project.

### `dev`
Starts the fullstack development environment. 

```bash
blaze dev
```
*   Compiles the C++ backend with a visual TUI.
*   Starts the Vite frontend development server.
*   **Unified Logging**: Merges C++ and Vite logs into a single stream, automatically filtering noise while highlighting the local URLs.

### `add`
Adds features to an existing Blaze project.

```bash
blaze add frontend
```
*   Injects a Vite-based frontend into a project that was originally created as backend-only.
*   Follows the same interactive framework selection as `blaze init --fullstack`.

### `run`
Builds and runs the backend project locally using the TUI builder.

```bash
blaze run
```
*   Compiles using `cmake`.
*   Shows a visual progress bar.
*   Launches the binary upon success.

### `doctor`
Checks your system for required dependencies.

```bash
blaze doctor
```
**Checks for:**
*   C++ Compiler (g++ / clang++)
*   CMake (3.20+)
*   OpenSSL Dev Libraries
*   Postgres/MySQL Dev Libraries

### `docker`
Manages the Docker-based development environment.

#### Subcommands

| Command | Description |
| :--- | :--- |
| `blaze docker psql` | Starts a PostgreSQL container (port 5432). |
| `blaze docker mysql` | Starts a MySQL container (port 3306). |
| `blaze docker redis` | Starts a Redis container (port 6379). |
| `blaze docker logs` | Streams logs from all running background containers. |
| `blaze docker stop` | Stops and removes all background containers. |
| `blaze docker build` | Builds the production Docker image for your app. |
| `blaze docker run` | Builds and runs your app inside a container (port 8080). |

## Environment Variables
The CLI respects the `.env` file generated in your project root for configuring Docker services.