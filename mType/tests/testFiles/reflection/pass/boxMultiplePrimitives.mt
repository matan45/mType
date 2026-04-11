// Test: Box<T> closed over each primitive wrapper reports the right
// type-argument name and keeps distinct handles per closed form.
// Exercises Box<Int>, Box<Float>, Box<Bool>, Box<String> in one pass.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Box.mt";

Class boxInt = Class::forName("Box<Int>");
Class boxFloat = Class::forName("Box<Float>");
Class boxBool = Class::forName("Box<Bool>");
Class boxString = Class::forName("Box<String>");

print("Box<Int> arg: " + boxInt.getTypeArguments()[0].getName());
print("Box<Float> arg: " + boxFloat.getTypeArguments()[0].getName());
print("Box<Bool> arg: " + boxBool.getTypeArguments()[0].getName());
print("Box<String> arg: " + boxString.getTypeArguments()[0].getName());

// Every closed form over Box shares the same raw name.
print("Int raw: " + boxInt.getRawName());
print("Float raw: " + boxFloat.getRawName());
print("Bool raw: " + boxBool.getRawName());
print("String raw: " + boxString.getRawName());

// Four distinct closed handles, one per canonical form.
int handleInt = boxInt.getNativeHandle();
int handleFloat = boxFloat.getNativeHandle();
int handleBool = boxBool.getNativeHandle();
int handleString = boxString.getNativeHandle();

print("Int!=Float: " + (handleInt != handleFloat));
print("Int!=Bool: " + (handleInt != handleBool));
print("Int!=String: " + (handleInt != handleString));
print("Float!=Bool: " + (handleFloat != handleBool));
print("Float!=String: " + (handleFloat != handleString));
print("Bool!=String: " + (handleBool != handleString));

print("Test passed");
