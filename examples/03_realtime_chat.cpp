/**
 * Example 03: Realtime Chat
 * 
 * This example demonstrates a simple WebSocket chat server.
 * Concepts:
 * - WebSocket route registration
 * - Event-based handlers (on_open, on_message, on_close)
 * - Path-based broadcasting (O(1) serialization)
 * - Automatic session management
 */

#include <blaze/app.h>
#include <blaze/websocket.h>

using namespace blaze;

int main() {
    App app;

    // 1. Register a WebSocket endpoint at /chat
    app.ws("/chat", {
        // Triggered when a new client connects
        .on_open = [&app](std::shared_ptr<WebSocket> ws) {
            std::cout << "[Chat] Client connected" << std::endl;
            ws->send("Welcome to the Blaze Chat!");
            
            // Notify others
            app.broadcast("/chat", "A new user joined the room.");
        },

        // Triggered when a client sends a message
        .on_message = [&app](auto ws, std::string msg) {
            std::cout << "[Chat] Received: " << msg << std::endl;
            
            // Broadcast the message to EVERYONE connected to /chat
            // Blaze serializes the message once and pushes it to all sockets
            app.broadcast("/chat", "User: " + msg);
        },

        // Triggered when a client disconnects
        .on_close = [&app](auto ws) {
            std::cout << "[Chat] Client disconnected" << std::endl;
            app.broadcast("/chat", "A user left the room.");
        }
    });

    // 2. Serve a simple HTML page to test the chat
    app.get("/", [](Response& res) -> Async<void> {
        res.send(R"HTML(
            <html>
                <body>
                    <h1>Blaze Chat</h1>
                    <div id="messages" style="height: 300px; overflow-y: scroll; border: 1px solid #ccc;"></div>
                    <input id="input" type="text" placeholder="Type a message..." />
                    <button id="sendBtn">Send</button>

                    <script>
                        const ws = new WebSocket("ws://" + location.host + "/chat");
                        const messages = document.getElementById("messages");
                        const input = document.getElementById("input");
                        const btn = document.getElementById("sendBtn");

                        ws.onmessage = (e) => {
                            const msg = document.createElement("div");
                            msg.textContent = e.data;
                            messages.appendChild(msg);
                            messages.scrollTop = messages.scrollHeight;
                        };

                        btn.onclick = () => {
                            ws.send(input.value);
                            input.value = "";
                        };
                    </script>
                </body>
            </html>
        )HTML");
        co_return;
    });

    std::cout << "Chat server running on http://localhost:8080" << std::endl;
    app.listen(8080);

    return 0;
}
