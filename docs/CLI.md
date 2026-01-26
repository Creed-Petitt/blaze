# Blaze CLI Reference

The Blaze CLI (`blaze`) is your all-in-one tool for scaffolding, building, and managing high-performance C++ web applications.

## Global Usage

```bash
blaze [command]
```

## Commands

### `init`
Scaffolds a new Blaze project with an interactive feature checklist.

```bash
blaze init <project-name>
```
**Interactive Options:**
- **PostgreSQL:** Injects the `blaze::postgres` driver.
- **MySQL:** Injects the `blaze::mysql` driver.
- **Catch2 Testing:** Configures a dedicated test suite.

**Flags:**
*   `--fullstack`, `-f`: Scaffolds a Vite-based frontend in the `/frontend` directory.

### `run`
Builds and runs the project locally. Defaults to **Debug** mode for fast compilation.

```bash
blaze run [--watch] [--release]
```
**Flags:**
*   `--watch`, `-w`: Enables **Hot-Reload**. Watches `src/` and `include/` for changes.
*   `--release`, `-r`: Runs in high-performance Release mode.

### `test`
Compiles and executes the project's Catch2 test suite.

```bash
blaze test
```
- Automatically configures the project if the build folder is missing.
- Provides a clean, green/red success summary.
- Streams raw assertion failures for instant debugging.

### `add`
Plugs new modules or drivers into your existing project.

```bash
blaze add mysql      # Wires up MySQL driver headers and libs
blaze add postgres   # Wires up PostgreSQL driver headers and libs
blaze add test       # Injects the Catch2 testing harness
blaze add frontend   # Adds a Vite-based frontend
```

### `build`
Performs a production-ready compilation.

```bash
blaze build [--debug]
```

### `doctor`
Verifies system dependencies (compilers, libs, Docker) and offers to fix missing ones.

```bash
blaze doctor [--fix]
```

### `dev`
Starts the full-stack development environment (Hot-reloaded Backend + Vite Frontend).

```bash
blaze dev
```

### `docs`
Displays the URLs for the automatic API documentation.

```bash
blaze docs
```
*   **Swagger UI:** `http://localhost:8080/docs`
*   **OpenAPI Spec:** `http://localhost:8080/openapi.json`

### `clean`
Wipes the `build/` directory and global compiler cache for the current project.

```bash
blaze clean
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
The CLI identifies common C++ mistakes (missing drivers, signature mismatches) and provides human-readable tips to fix them, saving you from parsing thousands of lines of template errors.
