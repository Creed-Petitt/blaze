# Hello World - BlazeBlus Example

The absolute minimum code needed to create a web server with BlazeBlus.

## Features

- Single file, ~20 lines of code
- Two routes: plain text and JSON
- No configuration needed

## Build & Run

```bash
cd examples/01-hello-world
mkdir build && cd build
cmake ..
make
./hello_world
```

## Test

```bash
# Plain text response
curl http://localhost:3000/

# JSON response
curl http://localhost:3000/json
```

## Expected Output

**GET /**
```
Hello, World!
```

**GET /json**
```json
{
  "message": "Hello from BlazeBlus!",
  "framework": "BlazeBlus",
  "language": "C++"
}
```

## Code Explanation

```cpp
App app;  // Create application instance

app.get("/", [](Request& req, Response& res) {
    res.send("Hello, World!");  // Send plain text
});

app.listen(3000);  // Start server on port 3000
```

That's it! You now have a working web server.

## Next Steps

- Check out [02-todo-api](../02-todo-api) for a full REST API example
- Check out [03-quote-generator](../03-quote-generator) for a full-stack app
