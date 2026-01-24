# Blaze CLI Reference

The Blaze CLI (`blaze`) is your all-in-one tool for scaffolding, building, and managing high-performance C++ web applications.

## Global Usage

```bash
blaze [command]
```

## Commands

### `init`
Scaffolds a new Blaze project. Projects are lean by default, linking only `blaze::core`.

```bash
blaze init <project-name> [--fullstack]
```
**Flags:**
*   `--fullstack`, `-f`: (Optional) Scaffolds a Vite-based frontend in the `/frontend` directory.

### `add`
Plugs new modules or drivers into your existing project.

```bash
blaze add mysql      # Wires up MySQL driver headers and libs
blaze add postgres   # Wires up PostgreSQL driver headers and libs
blaze add frontend   # Adds a Vite-based frontend to a backend-only project
```

### `build`
Compiles the project without running it. Defaults to **Release** mode for maximum performance.

```bash
blaze build
```
**Flags:**
*   `--debug`, `-d`: Compiles in Debug mode instead of Release.

### `run`
Builds and runs the backend project locally. Defaults to **Debug** mode for fast compilation.

```bash
blaze run
```
**Flags:**
*   `--release`, `-r`: Runs the project in high-performance Release mode.

### `dev`
Starts the fullstack development environment (Backend + Frontend).

```bash
blaze dev
```

### `clean`
Wipes the `build/` directory and CMake cache. Use this if you encounter persistent build errors.

```bash
blaze clean
```

### `doctor`
Checks your system for required compilers and libraries.

```bash
blaze doctor
```

### `docker`
Manages the Docker-based development environment.

| Command | Description |
| :--- | :--- |
| `blaze docker psql` | Starts a PostgreSQL container (port 5432). |
| `blaze docker mysql` | Starts a MySQL container (port 3306). |
| `blaze docker redis` | Starts a Redis container (port 6379). |
| `blaze docker logs` | Streams logs from background containers. |
| `blaze docker stop` | Stops all background containers. |

## Error Beautification
The CLI includes a built-in error translation engine. Instead of showing thousands of lines of C++ template errors, it identifies common mistakes (like missing drivers or handler mismatches) and provides human-readable advice and tips to fix them.
