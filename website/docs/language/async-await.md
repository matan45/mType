---
title: Async / Await
sidebar_position: 9
---

# Async / Await

mType supports asynchronous functions with `Promise<T>` values, scheduled cooperatively on the VM event loop.

## `async` Functions

Mark a function `async` to make it return a `Promise<T>`:

```mtype
function async getMessage(): Promise<Message> {
    Message msg = new Message("Hello from async");
    return msg;
}
```

The return type is wrapped: from inside the function you `return` a `Message`; callers receive a `Promise<Message>`.

## `await`

Inside another `async` function, use `await` to suspend until a promise resolves:

```mtype
function async testBasic(): Promise<Message> {
    Message result = await getMessage();
    return result;
}
```

## A Complete Example

```mtype
function async getMessage(): Promise<Message> {
    Message msg = new Message("Hello from async");
    return msg;
}

function async testBasic(): Promise<Message> {
    Message result = await getMessage();
    return result;
}

function async main(): Promise<Message> {
    Message msg = await testBasic();
    print("Result: " + msg.getText());
    return msg;
}
```

Top-level `main()` may itself be `async` — the VM runs the event loop until the returned promise settles.

## Concurrency Patterns

### Sequential

```mtype
A a = await stepA();
B b = await stepB(a);
return b;
```

### Mixing Sync and Async

A regular (non-`async`) function cannot use `await` directly. Wrap synchronous work in an `async` function or call the sync function before/after the await chain.

## See Also

- [Standard Library / Network](../stdlib/net.md) — async HTTP and TCP examples.
- [`mtest` Async Tests](../stdlib/mtest.md) — testing `async` code.
