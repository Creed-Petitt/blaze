# Blaze CLI Reference

The Blaze CLI (`blaze`) is your all-in-one tool for scaffolding, building, and managing C++ web applications.

## Global Usage

```bash
blaze [command]
```

## Commands

### `init`
Scaffolds a new Blaze project with a standardized directory structure.

```bash
blaze init <project-name>
```
**Creates:**
*   `src/main.cpp`: Entry point.
*   `CMakeLists.txt`: Build configuration (fetches Blaze automatically).
*   `Dockerfile`: Multi-stage build for production.
*   `docker-compose.yml`: Development environment (Postgres/Redis/MySQL).

### `run`
Builds and runs the project locally using the TUI builder.

```bash
cd <project-name>
blaze run
```
*   Compiles using `cmake` (RelWithDebInfo by default).
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
