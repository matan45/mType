// Module: Real-world service interfaces

interface AsyncUserService {
    async authenticate(username: String, password: String) : Promise<Bool>;
    async getUserData(userId: Int) : Promise<String>;
}

interface AsyncDataService<T> {
    async load(id: Int) : Promise<T>;
    async save(id: Int, data: T) : Promise<Bool>;
    async query(filter: (T) -> Bool) : Promise<T[]>;
}

interface AsyncLogger {
    async logInfo(message: String) : Promise<Void>;
    async logError(message: String) : Promise<Void>;
}
