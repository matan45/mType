// @Builder emits a companion ConfigBuilder class with fluent field setters and
// a build() that calls the (auto-synthesized) all-args constructor, plus a
// static Config::builder() factory. @Getter lets us read the result back.
@Builder
@Getter
class Config {
    private int port;
    private string host;
}

Config c = Config::builder().port(8080).host("localhost").build();
print(c.getPort());
print(c.getHost());
