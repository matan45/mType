// Test package-private interfaces (if supported)
// @Script

// This test assumes package-private is the default or explicitly supported
// If not supported, interface should still work as public

interface InternalService {
    func execute(): void;
}

class ServiceImpl implements InternalService {
    func execute(): void {
        print("Service executing");
    }
}

// Within same package/file, should work fine
var service: InternalService = new ServiceImpl();
service.execute();

// Test that implementation is accessible
class ServiceManager {
    var service: InternalService;

    func init(service: InternalService) {
        this.service = service;
    }

    func run(): void {
        this.service.execute();
    }
}

var manager = new ServiceManager(new ServiceImpl());
manager.run();
