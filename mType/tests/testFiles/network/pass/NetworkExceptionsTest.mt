// Unit tests for the Network*Exception hierarchy. Verifies the inheritance
// chain and toString prefixes used by mtest's assertThrows matcher.
import * from "../../lib/mtest/Mtest.mt";
import * from "../../lib/net/exceptions/NetworkException.mt";
import * from "../../lib/net/exceptions/ConnectionException.mt";
import * from "../../lib/net/exceptions/TimeoutException.mt";
import * from "../../lib/net/exceptions/DnsException.mt";
import * from "../../lib/net/exceptions/HttpException.mt";

public class NetworkExceptionsTest extends TestSuite {
    public constructor() : super() { }

    @Test
    public function testNetworkExceptionMessage(): void {
        NetworkException e = new NetworkException("net broke");
        assertEqual(e.getMessage(), "net broke");
        assertEqual(e.toString(), "NetworkException: net broke");
    }

    @Test
    public function testConnectionExceptionPrefix(): void {
        ConnectionException e = new ConnectionException("refused");
        assertEqual(e.toString(), "ConnectionException: refused");
    }

    @Test
    public function testTimeoutExceptionPrefix(): void {
        TimeoutException e = new TimeoutException("slow");
        assertEqual(e.toString(), "TimeoutException: slow");
    }

    @Test
    public function testDnsExceptionPrefix(): void {
        DnsException e = new DnsException("nxdomain");
        assertEqual(e.toString(), "DnsException: nxdomain");
    }

    @Test
    public function testHttpExceptionStatusField(): void {
        HttpException e = new HttpException("not found");
        e.status = 404;
        assertEqual(e.status, 404);
        assertEqual(e.toString(), "HttpException(404): not found");
    }

    @Test(expected = "NetworkException")
    public function testThrowAndCatch(): void {
        throw new NetworkException("boom");
    }
}
