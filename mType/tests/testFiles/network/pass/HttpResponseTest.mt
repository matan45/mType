// Unit tests for the HttpResponse value class. No network access.
import * from "../../lib/mtest/Mtest.mt";
import * from "../../lib/net/HttpResponse.mt";
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/String.mt";

public class HttpResponseTest extends TestSuite {
    public constructor() : super() { }

    @Test
    public function testIsOkInRange(): void {
        HashMap<String, String> h = new HashMap<String, String>();
        HttpResponse r200 = new HttpResponse(200, "ok", h);
        HttpResponse r299 = new HttpResponse(299, "ok", h);
        HttpResponse r300 = new HttpResponse(300, "redir", h);
        HttpResponse r404 = new HttpResponse(404, "missing", h);

        assertTrue(r200.isOk(), "200 should be ok");
        assertTrue(r299.isOk(), "299 should be ok");
        assertFalse(r300.isOk(), "300 should not be ok");
        assertFalse(r404.isOk(), "404 should not be ok");
    }

    @Test
    public function testHeaderAccessor(): void {
        HashMap<String, String> h = new HashMap<String, String>();
        h.put(new String("Content-Type"), new String("application/json"));
        h.put(new String("X-Custom"), new String("abc"));
        HttpResponse r = new HttpResponse(200, "{}", h);

        assertEqual(r.header("Content-Type"), "application/json");
        assertEqual(r.header("X-Custom"), "abc");
        assertEqual(r.header("Missing"), "");
    }

    @Test
    public function testFieldsExposed(): void {
        HashMap<String, String> h = new HashMap<String, String>();
        HttpResponse r = new HttpResponse(201, "created", h);
        assertEqual(r.status, 201);
        assertEqual(r.body, "created");
    }

    @Test
    public function testDefaultConstructor(): void {
        HttpResponse r = new HttpResponse();
        assertEqual(r.status, 0);
        assertEqual(r.body, "");
        assertNotNull(r.headers);
        assertFalse(r.isOk());
    }
}
