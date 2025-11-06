# Todo REST API

A complete CRUD (Create, Read, Update, Delete) REST API demonstrating all major framework features.

## Features Demonstrated

- ✅ Route grouping (`/api` prefix)
- ✅ All HTTP methods (GET, POST, PUT, DELETE)
- ✅ Path parameters (`:id`)
- ✅ Query parameters (`?status=completed`)
- ✅ Request/response helpers
- ✅ Input validation
- ✅ Error handling
- ✅ Automatic JSON serialization
- ✅ CORS middleware

## Build & Run

```bash
cd examples/02-todo-api
mkdir build && cd build
cmake ..
make
./todo_api
```

## API Endpoints

### List All Todos
```bash
curl http://localhost:3000/api/todos
```

**Filter by status:**
```bash
# Get completed todos
curl "http://localhost:3000/api/todos?status=completed"

# Get active todos
curl "http://localhost:3000/api/todos?status=active"
```

### Get Single Todo
```bash
curl http://localhost:3000/api/todos/1
```

### Create Todo
```bash
curl -X POST http://localhost:3000/api/todos \
  -H "Content-Type: application/json" \
  -d '{"title":"New task","completed":false}'
```

### Update Todo
```bash
# Update title
curl -X PUT http://localhost:3000/api/todos/1 \
  -H "Content-Type: application/json" \
  -d '{"title":"Updated title"}'

# Mark as completed
curl -X PUT http://localhost:3000/api/todos/1 \
  -H "Content-Type: application/json" \
  -d '{"completed":true}'
```

### Delete Todo
```bash
curl -X DELETE http://localhost:3000/api/todos/1
```

## Error Handling

The API returns appropriate HTTP status codes and error messages:

```bash
# Invalid ID (not a number)
curl http://localhost:3000/api/todos/abc
# Returns: 400 Bad Request
# {"error":"Invalid todo ID - must be a number"}

# Todo not found
curl http://localhost:3000/api/todos/999
# Returns: 404 Not Found
# {"error":"Todo not found"}

# Missing required field
curl -X POST http://localhost:3000/api/todos \
  -H "Content-Type: application/json" \
  -d '{}'
# Returns: 400 Bad Request
# {"error":"Title is required and cannot be empty"}
```

## Code Highlights

### Automatic JSON Serialization
```cpp
struct Todo {
    int id;
    std::string title;
    bool completed;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Todo, id, title, completed)

// Now you can do:
res.json(todo);  // Automatically serialized!
```

### Request Helpers
```cpp
// Safe parameter parsing with defaults
auto id = req.get_param_int("id");  // Returns std::optional<int>
std::string filter = req.get_query("status", "all");  // With default
```

### Response Helpers
```cpp
res.bad_request("Invalid input");  // 400
res.not_found("Todo not found");   // 404
res.no_content();                  // 204
```

## Next Steps

- Add database persistence (SQLite)
- Add authentication
- Add pagination
- Check out [03-quote-generator](../03-quote-generator) for a full-stack example
