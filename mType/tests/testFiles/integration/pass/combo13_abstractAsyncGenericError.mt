// Combo 13: Abstract Class + Async + Generic Constraints + Errors

import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Validatable {
    function isValid(): bool;
    function validate(): string;
}

class ValidationException extends Exception {
    private string field;

    public constructor(string field, string msg) : super(msg) {
        this.field = field;
    }

    public function getField(): string {
        return this.field;
    }
}

abstract class AsyncProcessor<T extends Validatable> {
    protected string processorName;

    public constructor(string name) {
        this.processorName = name;
    }

    public abstract function async process(T item): Promise<String>;

    public function async validateAndProcess(T item): Promise<String> {
        if (!item.isValid()) {
            throw new ValidationException(this.processorName, item.validate());
        }
        return await this.process(item);
    }

    public function getName(): string {
        return this.processorName;
    }
}

class Email implements Validatable {
    private string address;

    public constructor(string address) {
        this.address = address;
    }

    public function isValid(): bool {
        // Simple validation: must be longer than 5 chars
        return strLength(this.address) > 5;
    }

    public function validate(): string {
        if (!this.isValid()) {
            return "Invalid email: too short";
        }
        return "OK";
    }

    public function getAddress(): string { return this.address; }
}

class PhoneNumber implements Validatable {
    private string number;

    public constructor(string number) {
        this.number = number;
    }

    public function isValid(): bool {
        return strLength(this.number) >= 7;
    }

    public function validate(): string {
        if (!this.isValid()) {
            return "Phone too short: " + strLength(this.number) + " digits";
        }
        return "OK";
    }

    public function getNumber(): string { return this.number; }
}

class EmailProcessor extends AsyncProcessor<Email> {
    public constructor() : super("EmailProcessor") {}

    public function async process(Email item): Promise<String> {
        return new String("Sent to: " + item.getAddress());
    }
}

class SMSProcessor extends AsyncProcessor<PhoneNumber> {
    public constructor() : super("SMSProcessor") {}

    public function async process(PhoneNumber item): Promise<String> {
        return new String("SMS to: " + item.getNumber());
    }
}

function async main(): Promise<void> {
    print("=== Combo 13: Abstract + Async + Generic Constraint + Error ===");

    print("--- Email processing ---");
    EmailProcessor emailProc = new EmailProcessor();
    Email validEmail = new Email("user@example.com");
    Email invalidEmail = new Email("ab");

    try {
        String r1 = await emailProc.validateAndProcess(validEmail);
        print(r1.getValue());
    } catch (ValidationException e) {
        print("Error: " + e.getMessage());
    }

    try {
        String r2 = await emailProc.validateAndProcess(invalidEmail);
        print(r2.getValue());
    } catch (ValidationException e) {
        print("Validation failed [" + e.getField() + "]: " + e.getMessage());
    }

    print("--- SMS processing ---");
    SMSProcessor smsProc = new SMSProcessor();
    PhoneNumber validPhone = new PhoneNumber("1234567890");
    PhoneNumber shortPhone = new PhoneNumber("123");

    try {
        String r3 = await smsProc.validateAndProcess(validPhone);
        print(r3.getValue());
    } catch (ValidationException e) {
        print("Error: " + e.getMessage());
    }

    try {
        String r4 = await smsProc.validateAndProcess(shortPhone);
        print(r4.getValue());
    } catch (ValidationException e) {
        print("Validation failed [" + e.getField() + "]: " + e.getMessage());
    }

    print("--- Processor names ---");
    print(emailProc.getName());
    print(smsProc.getName());

    print("=== Combo 13 Complete ===");
}

main();
