// Module: Generic repository interface

interface Repository<T> {
    save(item: T) : Void;
    findById(id: Int) : T;
    count() : Int;
}
