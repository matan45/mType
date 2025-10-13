import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

function async getInt():Promise<Int>{
return new Int(42);
}

function async getString():Promise<String>{
return new String("async result");
}


function async handleIntPromise(Promise<Int> p): Promise<Int> {
    return await p;
}

function async handleStringPromise(Promise<String> p): Promise<String> {
    return await p;
}

// Create promises
Promise<Int> intPromise = getInt();
Promise<String> strPromise = getString();

// Call functions
  print((await handleIntPromise(intPromise)).toString());
  print((await handleStringPromise(strPromise)).toString());

