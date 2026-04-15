// Unit tests for JsonApi configuration (URL building, header defaults). These
// do not perform any network I/O; they verify the wiring that the request
// path uses before crossing into native code.
import * from "../../lib/mtest/Mtest.mt";
import * from "../../lib/net/JsonApi.mt";
import * from "../../lib/net/HttpResponse.mt";
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/String.mt";

public class JsonApiTest extends TestSuite {
    public constructor() : super() { }

    @Test
    public function testBaseUrlStored(): void {
        JsonApi api = new JsonApi("http://example.com");
        assertEqual(api.baseUrl, "http://example.com");
    }

    @Test
    public function testDefaultAcceptHeader(): void {
        JsonApi api = new JsonApi("http://example.com");
        String v = api.defaultHeaders.get(new String("Accept"));
        assertNotNull(v);
        assertEqual(v.getValue(), "application/json");
    }

    @Test
    public function testSetHeaderChains(): void {
        JsonApi api = new JsonApi("http://example.com");
        JsonApi same = api.setHeader("Authorization", "Bearer xyz");
        String v = api.defaultHeaders.get(new String("Authorization"));
        assertNotNull(v);
        assertEqual(v.getValue(), "Bearer xyz");
        assertNotNull(same);
    }

    @Test
    public function testSetTimeoutChains(): void {
        JsonApi api = new JsonApi("http://example.com");
        JsonApi same = api.setTimeout(5000);
        assertEqual(api.timeoutMs, 5000);
        assertNotNull(same);
    }

    @Test
    public function testDefaultTimeout(): void {
        JsonApi api = new JsonApi("http://example.com");
        assertEqual(api.timeoutMs, 30000);
    }
}
