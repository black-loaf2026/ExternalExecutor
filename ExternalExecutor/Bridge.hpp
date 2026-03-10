#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "Dependecies/server/httplib.h"
using namespace httplib;

#include "Dependecies/server/nlohmann/json.hpp"
using json = nlohmann::json;

#include <regex>
#include <filesystem>
#include <fstream>

#include "Utils/Process.hpp"
#include "Utils/Instance.hpp"
#include "Utils/Bytecode.hpp"

inline std::string script = "";
inline uintptr_t order = 0;
inline std::unordered_map<DWORD, uintptr_t> orders;

inline std::vector<std::string> SplitLines(const std::string& str) {
	std::stringstream ss(str);
	std::string line;
	std::vector<std::string> lines;
	while (std::getline(ss, line, '\n'))
		lines.push_back(line);
	return lines;
}

inline Instance GetPointerInstance(std::string name, DWORD ProcessID) {
	uintptr_t Base = Process::GetModuleBase(ProcessID);
	Instance Datamodel = FetchDatamodel(Base, ProcessID);
	Instance CoreGui = Datamodel.FindFirstChild("CoreGui");
	Instance ExternalExecutor = CoreGui.FindFirstChild("ExternalExecutor");
	Instance Pointers = ExternalExecutor.FindFirstChild("Pointer");
	Instance Pointer = Pointers.FindFirstChild(name);
	uintptr_t Target = ReadMemory<uintptr_t>(Pointer.GetAddress() + Offsets::Value, ProcessID);
	return Instance(Target, ProcessID);
}

inline std::unordered_map<std::string, std::function<std::string(std::string, nlohmann::json, DWORD)>> env;
inline void Load() {
	env["listen"] = [](std::string dta, nlohmann::json set, DWORD pid) {
		std::string res;
		if (orders.contains(pid)) {
			if (orders[pid] < order) {
				res = script;
			}
			else {
				res = "";
			}
			orders[pid] = order;
		}
		else {
			orders[pid] = order;
			res = script;
		}
		return res;
		};
	env["compile"] = [](std::string dta, nlohmann::json set, DWORD pid) {
		if (set["enc"] == "true") {
			return Bytecode::Compile(dta);
		}
		return Bytecode::NormalCompile(dta);
		};
	env["setscriptbytecode"] = [](std::string dta, nlohmann::json set, DWORD pid) {
		size_t Sized;
		auto Compressed = Bytecode::Sign(dta, Sized);

		Instance TheScript = GetPointerInstance(set["cn"], pid);
		TheScript.SetScriptBytecode(Compressed, Sized);

		return "";
		};
	env["request"] = [](std::string dta, nlohmann::json set, DWORD pid) {
		std::string url = set["l"];
		std::string method = set["m"];
		std::string rBody = set["b"];
		json headersJ = set["h"];

		std::regex urlR(R"(^(http[s]?:\/\/)?([^\/]+)(\/.*)?$)");
		std::smatch urlM;
		std::string host;
		std::string path = "/";

		if (std::regex_match(url, urlM, urlR)) {
			host = urlM[2];
			if (urlM[3].matched) path = urlM[3];
		}
		else {
			return std::string("[]");
		}

		Client client(host.c_str());
		client.set_follow_location(true);

		Headers headers;
		for (auto it = headersJ.begin(); it != headersJ.end(); ++it) {
			headers.insert({ it.key(), it.value() });
		}

		Result proxiedRes;
		if (method == "GET") {
			proxiedRes = client.Get(path, headers);
		}
		else if (method == "POST") {
			proxiedRes = client.Post(path, headers, rBody, "application/json");
		}
		else if (method == "PUT") {
			proxiedRes = client.Put(path, headers, rBody, "application/json");
		}
		else if (method == "DELETE") {
			proxiedRes = client.Delete(path, headers, rBody, "application/json");
		}
		else if (method == "PATCH") {
			proxiedRes = client.Patch(path, headers, rBody, "application/json");
		}
		else {
			return std::string("[]");
		}

		if (proxiedRes) {
			json responseJ;
			responseJ["b"] = proxiedRes->body;
			responseJ["c"] = proxiedRes->status;
			responseJ["r"] = proxiedRes->reason;
			responseJ["v"] = proxiedRes->version;

			json rHeadersJ;
			for (const auto& header : proxiedRes->headers) {
				rHeadersJ[header.first] = header.second;
			}
			responseJ["h"] = rHeadersJ;

			return responseJ.dump();
		}
		return std::string("[]");
		};
}

inline std::string Setup(std::string args) {
	auto lines = SplitLines(args);

	std::string typ = lines.size() > 0 ? lines[0] : "";
	DWORD pid = lines.size() > 1 ? std::stoul(lines[1]) : 0;
	nlohmann::json set = lines.size() > 2 ? nlohmann::json::parse(lines[2]) : nlohmann::json{};
	std::string dta;

	for (size_t i = 3; i < lines.size(); ++i) {
		dta += lines[i];
		if (i + 1 < lines.size()) dta += "\n";
	}

	return env[typ] ? env[typ](dta, set, pid) : "";
}

inline void StartBridge()
{
	Load();
	Server Bridge;
	Bridge.Post("/handle", [](const Request& req, Response& res) {
		res.status = 200;
		res.set_content(Setup(req.body), "text/plain");
		});
	Bridge.set_exception_handler([](const Request& req, Response& res, std::exception_ptr ep) {
		std::string errorMessage;
		try {
			std::rethrow_exception(ep);
		}
		catch (std::exception& e) {
			errorMessage = e.what();
		}
		catch (...) {
			errorMessage = "Unknown Exception";
		}
		res.set_content("{\"error\":\"" + errorMessage + "\"}", "application/json");
		res.status = 500;
		});
	Bridge.listen("localhost", 9611);
}

inline void Execute(std::string source) {
	script = source;
	order += 1;
}