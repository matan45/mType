#pragma once

#include <variant>
#include <string>
#include <memory>
#include <optional>
#include <functional>
#include <stdexcept>

namespace mtype::core
{

	// Error types for the Result pattern
	enum class ErrorType
	{
		NONE,
		SYNTAX_ERROR,
		RUNTIME_ERROR,
		TYPE_ERROR,
		NAME_ERROR,
		INDEX_ERROR,
		ARGUMENT_ERROR,
		FILE_ERROR,
		MODULE_ERROR,
		ACCESS_ERROR
	};

	struct Error
	{
		ErrorType type;
		std::string message;
		int line;
		std::string file;

		Error() : type(ErrorType::NONE), line(-1)
		{
		}

		Error(ErrorType t, const std::string& msg, int l = -1, const std::string& f = "")
			: type(t), message(msg), line(l), file(f)
		{
		}

		static Error syntax(const std::string& msg, int line = -1)
		{
			return Error(ErrorType::SYNTAX_ERROR, msg, line);
		}

		static Error runtime(const std::string& msg, int line = -1)
		{
			return Error(ErrorType::RUNTIME_ERROR, msg, line);
		}

		static Error errorType(const std::string& msg, int line = -1)
		{
			return Error(ErrorType::TYPE_ERROR, msg, line);
		}

		static Error name(const std::string& msg, int line = -1)
		{
			return Error(ErrorType::NAME_ERROR, msg, line);
		}

		static Error index(const std::string& msg, int line = -1)
		{
			return Error(ErrorType::INDEX_ERROR, msg, line);
		}

		static Error argument(const std::string& msg, int line = -1)
		{
			return Error(ErrorType::ARGUMENT_ERROR, msg, line);
		}

		static Error access(const std::string& msg, int line = -1)
		{
			return Error(ErrorType::ACCESS_ERROR, msg, line);
		}

		std::string toString() const
		{
			std::string result = "[";
			switch (type)
			{
			case ErrorType::SYNTAX_ERROR: result += "SyntaxError";
				break;
			case ErrorType::RUNTIME_ERROR: result += "RuntimeError";
				break;
			case ErrorType::TYPE_ERROR: result += "TypeError";
				break;
			case ErrorType::NAME_ERROR: result += "NameError";
				break;
			case ErrorType::INDEX_ERROR: result += "IndexError";
				break;
			case ErrorType::ARGUMENT_ERROR: result += "ArgumentError";
				break;
			case ErrorType::FILE_ERROR: result += "FileError";
				break;
			case ErrorType::MODULE_ERROR: result += "ModuleError";
				break;
			case ErrorType::ACCESS_ERROR: result += "AccessError";
				break;
			default: result += "Error";
				break;
			}

			if (line >= 0)
			{
				result += " at line " + std::to_string(line);
			}
			if (!file.empty())
			{
				result += " in " + file;
			}
			result += "] " + message;
			return result;
		}
	};

	template<typename T>
	class Result {
	private:
		std::variant<T, Error> data;

	public:
		// Constructors
		explicit Result(const T& value) : data(value) {}
		explicit Result(T&& value) : data(std::move(value)) {}
		explicit Result(const Error& error) : data(error) {}
		explicit Result(Error&& error) : data(std::move(error)) {}

		// Static factory methods
		static Result<T> ok(const T& value) { return Result(value); }
		static Result<T> ok(T&& value) { return Result(std::move(value)); }
		static Result<T> err(const Error& error) { return Result(error); }
		static Result<T> err(Error&& error) { return Result(std::move(error)); }

		// Check if result is ok or error
		bool isOk() const { return std::holds_alternative<T>(data); }
		bool isError() const { return std::holds_alternative<Error>(data); }

		// Get value (throws if error)
		T& value() {
			if (isError()) {
				throw std::runtime_error(error().toString());
			}
			return std::get<T>(data);
		}

		const T& value() const {
			if (isError()) {
				throw std::runtime_error(error().toString());
			}
			return std::get<T>(data);
		}

		// Get value or default
		T valueOr(const T& defaultValue) const {
			return isOk() ? std::get<T>(data) : defaultValue;
		}

		// Get error (throws if ok)
		Error& error() {
			if (isOk()) {
				throw std::runtime_error("Result is not an error");
			}
			return std::get<Error>(data);
		}

		const Error& error() const {
			if (isOk()) {
				throw std::runtime_error("Result is not an error");
			}
			return std::get<Error>(data);
		}

		// Monadic operations
		template<typename U>
		Result<U> map(std::function<U(const T&)> fn) const {
			if (isOk()) {
				return Result<U>::ok(fn(std::get<T>(data)));
			}
			return Result<U>::err(std::get<Error>(data));
		}

		template<typename U>
		Result<U> andThen(std::function<Result<U>(const T&)> fn) const {
			if (isOk()) {
				return fn(std::get<T>(data));
			}
			return Result<U>::err(std::get<Error>(data));
		}

		Result<T> orElse(std::function<Result<T>(const Error&)> fn) const {
			if (isError()) {
				return fn(std::get<Error>(data));
			}
			return *this;
		}

		// Unwrap with custom error message
		T expect(const std::string& msg) const {
			if (isError()) {
				throw std::runtime_error(msg + ": " + error().toString());
			}
			return std::get<T>(data);
		}
	};

	// Specialized Result for void operations
	template<>
	class Result<void> {
	private:
		std::optional<Error> error_;

	public:
		Result() : error_(std::nullopt) {}
		explicit Result(const Error& error) : error_(error) {}
		explicit Result(Error&& error) : error_(std::move(error)) {}

		static Result<void> ok() { return Result(); }
		static Result<void> err(const Error& error) { return Result(error); }
		static Result<void> err(Error&& error) { return Result(std::move(error)); }

		bool isOk() const { return !error_.has_value(); }
		bool isError() const { return error_.has_value(); }

		const Error& error() const {
			if (!error_.has_value()) {
				throw std::runtime_error("Result is not an error");
			}
			return error_.value();
		}

		void expect(const std::string& msg) const {
			if (isError()) {
				throw std::runtime_error(msg + ": " + error().toString());
			}
		}
	};

}
