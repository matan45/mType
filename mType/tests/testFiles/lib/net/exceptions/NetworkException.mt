import * from "../../exceptions/Exception.mt";

class NetworkException extends Exception {
    public constructor(string msg) : super(msg) {
    }

    public function toString(): string {
        return "NetworkException: " + message;
    }
}
