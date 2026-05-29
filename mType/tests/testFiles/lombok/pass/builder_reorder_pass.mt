// @Builder fluent setters are order-independent and cover all field types.
// Here they're called in a different order than declared, with an int, a
// string, and a bool field.
@Builder
@Getter
class Server {
    private string name;
    private int port;
    private bool secure;
}

Server s = Server::builder().port(443).secure(true).name("api").build();
print(s.getName());
print(s.getPort());
print(s.getSecure());
