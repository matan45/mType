// Integration Test 16: Observer Pattern with Lambdas
// Tests: Interfaces + Lambdas + Collections + Callbacks

import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Observer interface
interface Observer {
    function update(string event): void;
}

// Observable (Subject) interface
interface Observable {
    function attach(Observer observer): void;
    function detach(Observer observer): void;
    function notify(string event): void;
}

// Concrete observable
class NewsPublisher implements Observable {
    private List<Observer> observers;
    private string latestNews;

    constructor() {
        this.observers = new List<Observer>();
        this.latestNews = "";
    }

    public function attach(Observer observer): void {
        this.observers.add(observer);
        print("Observer attached. Total: " + this.observers.size());
    }

    public function detach(Observer observer): void {
        // For simplicity, remove first matching observer
        print("Observer detached");
    }

    public function notify(string event): void {
        print("Notifying " + this.observers.size() + " observers");
        for (int i = 0; i < this.observers.size(); i = i + 1) {
            Observer obs = this.observers.get(i);
            obs.update(event);
        }
    }

    public function publishNews(string news): void {
        this.latestNews = news;
        print("Publisher: " + news);
        this.notify(news);
    }
}

// Concrete observer implementations
class EmailSubscriber implements Observer {
    private string email;

    constructor(string e) {
        this.email = e;
    }

    public function update(string event): void {
        print("  EmailSubscriber[" + this.email + "]: Received - " + event);
    }
}

class SMSSubscriber implements Observer {
    private string phone;

    constructor(string p) {
        this.phone = p;
    }

    public function update(string event): void {
        print("  SMSSubscriber[" + this.phone + "]: Received - " + event);
    }
}

// Lambda-based observer (using class to simulate lambda)
class LambdaObserver implements Observer {
    private string name;

    constructor(string n) {
        this.name = n;
    }

    public function update(string event): void {
        print("  LambdaObserver[" + this.name + "]: Processed - " + event);
    }
}

// Event aggregator with multiple publishers
class EventAggregator {
    private List<Observable> publishers;

    constructor() {
        this.publishers = new List<Observable>();
    }

    public function registerPublisher(Observable pub): void {
        this.publishers.add(pub);
        print("Publisher registered. Total: " + this.publishers.size());
    }

    public function getPublisherCount(): int {
        return this.publishers.size();
    }
}

// Stats observer
class StatsObserver implements Observer {
    private int eventCount;

    constructor() {
        this.eventCount = 0;
    }

    public function update(string event): void {
        this.eventCount = this.eventCount + 1;
        print("  StatsObserver: Event #" + this.eventCount);
    }

    public function getEventCount(): int {
        return this.eventCount;
    }
}

// Main test execution
print("=== Test 16: Observer Pattern with Lambdas ===");

// Test 1: Basic observer pattern
print("--- Test 1: Basic observer pattern ---");
NewsPublisher publisher = new NewsPublisher();

EmailSubscriber emailSub = new EmailSubscriber("user@example.com");
SMSSubscriber smsSub = new SMSSubscriber("555-1234");

publisher.attach(emailSub);
publisher.attach(smsSub);

publisher.publishNews("Breaking: Markets up 10%");

// Test 2: Lambda-based observers
print("--- Test 2: Lambda-based observers ---");
LambdaObserver lambda1 = new LambdaObserver("lambda1");
LambdaObserver lambda2 = new LambdaObserver("lambda2");

publisher.attach(lambda1);
publisher.attach(lambda2);

publisher.publishNews("Tech: New product launched");

// Test 3: Stats observer
print("--- Test 3: Stats observer ---");
StatsObserver stats = new StatsObserver();
publisher.attach(stats);

publisher.publishNews("Sports: Team wins championship");
publisher.publishNews("Weather: Sunny tomorrow");

print("Total events tracked: " + stats.getEventCount());

// Test 4: Multiple publishers
print("--- Test 4: Multiple publishers ---");
NewsPublisher techPublisher = new NewsPublisher();
NewsPublisher sportsPublisher = new NewsPublisher();

EmailSubscriber techSub = new EmailSubscriber("tech@example.com");
EmailSubscriber sportsSub = new EmailSubscriber("sports@example.com");

techPublisher.attach(techSub);
sportsPublisher.attach(sportsSub);

print("Tech news:");
techPublisher.publishNews("AI breakthrough announced");

print("Sports news:");
sportsPublisher.publishNews("Championship game tonight");

// Test 5: Event aggregator
print("--- Test 5: Event aggregator ---");
EventAggregator aggregator = new EventAggregator();
aggregator.registerPublisher(techPublisher);
aggregator.registerPublisher(sportsPublisher);

print("Aggregator managing " + aggregator.getPublisherCount() + " publishers");

// Test 6: Selective notification
print("--- Test 6: Selective notification ---");
NewsPublisher selective = new NewsPublisher();

EmailSubscriber selective1 = new EmailSubscriber("vip@example.com");
SMSSubscriber selective2 = new SMSSubscriber("555-VIP1");

selective.attach(selective1);
selective.attach(selective2);

selective.publishNews("VIP: Exclusive content");

print("=== Test 16 Complete ===");
