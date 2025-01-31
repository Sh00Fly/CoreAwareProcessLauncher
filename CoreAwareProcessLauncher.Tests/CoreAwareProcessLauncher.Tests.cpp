//CoreAwareProcessLauncher.Tests.cpp
#include "pch.h"
#include "options.h"
#include "logger.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreAwareProcessLauncherTests
{
	TEST_CLASS(OptionsTests)
	{
	private:
		// Vector to store our allocated strings
		std::vector<wchar_t*> allocated_strings;

		// Helper to convert string arguments to argc/argv format
		std::pair<int, wchar_t**> PrepareArgs(const std::vector<std::wstring>& args) {
			// Clear any previously allocated strings
			for (auto ptr : allocated_strings) {
				free(ptr);
			}
			allocated_strings.clear();

			// Allocate new array of pointers (+1 for program name)
			wchar_t** argv = new wchar_t* [args.size() + 1];

			// Add program name as first argument
			argv[0] = _wcsdup(L"program.exe");
			allocated_strings.push_back(argv[0]);

			// Add other arguments
			for (size_t i = 0; i < args.size(); ++i) {
				argv[i + 1] = _wcsdup(args[i].c_str());
				allocated_strings.push_back(argv[i + 1]);
			}

			return { static_cast<int>(args.size() + 1), argv };
		}

		void CleanupArgs(wchar_t** argv) {
			// Free the array of pointers
			delete[] argv;

			// Free the allocated strings
			for (auto ptr : allocated_strings) {
				free(ptr);
			}
			allocated_strings.clear();
		}

		// logger initialization
		void InitializeLogger()
		{
			if (!g_logger) {
				g_logger = std::make_unique<ApplicationLogger>(false, L"test.log");
			}
		}

		std::wstring GetTestExecutablePath() {
			return L"TestExecutable.exe";  // Since it's in the same directory as the test DLL
		}

	public:

		TEST_METHOD_INITIALIZE(SetUp)
		{
			InitializeLogger();
		}

		TEST_METHOD_CLEANUP(TearDown)
		{
			g_logger.reset();
		}

		TEST_METHOD(TestDefaultConstructor)
		{
			CommandLineOptions opts;

			// Verify default values
			Assert::IsTrue(opts.targetPath.empty());
			Assert::AreEqual(static_cast<int>(CommandLineOptions::SelectionMethod::NONE),
				static_cast<int>(opts.selectionMethod));
			Assert::IsFalse(opts.invertSelection);
			Assert::IsFalse(opts.queryMode);
			Assert::IsFalse(opts.showHelp);
		}

		TEST_METHOD(TestArgumentPreparation)
		{
			Logger::WriteMessage(L"Starting TestArgumentPreparation\n");

			std::vector<std::wstring> testArgs = {
				L"--target", L"test.exe"
			};

			auto [argc, argv] = PrepareArgs(testArgs);

			Assert::AreEqual(3, argc); // program name + 2 args
			Assert::IsNotNull(argv);
			Assert::IsNotNull(argv[0]);
			Assert::IsNotNull(argv[1]);
			Assert::IsNotNull(argv[2]);

			std::wstring argCount = L"argc: " + std::to_wstring(argc) + L"\n";
			Logger::WriteMessage(argCount.c_str());

			for (int i = 0; i < argc; i++) {
				std::wstring argStr = L"argv[" + std::to_wstring(i) + L"]: " + argv[i] + L"\n";
				Logger::WriteMessage(argStr.c_str());
			}

			Assert::AreEqual(L"program.exe", argv[0]);
			Assert::AreEqual(L"--target", argv[1]);
			Assert::AreEqual(L"test.exe", argv[2]);

			CleanupArgs(argv);
		}
		TEST_METHOD(TestShowHelpWhenNoArgs)
		{
			auto [argc, argv] = PrepareArgs({});
			auto options = ParseCommandLine(argc, argv);

			Assert::IsTrue(options.showHelp);
			CleanupArgs(argv);
		}

		TEST_METHOD(TestBasicTargetWithPCoreDetection)
		{
			Logger::WriteMessage(L"Starting TestBasicTargetWithPCoreDetection\n");

			auto [argc, argv] = PrepareArgs({
				L"--target", L"TestExecutable.exe",  // Changed this
				L"--detect", L"P"
				});

			Logger::WriteMessage(L"Arguments prepared\n");

			std::wstring argCount = L"argc: " + std::to_wstring(argc) + L"\n";
			Logger::WriteMessage(argCount.c_str());

			for (int i = 0; i < argc; i++) {
				std::wstring argStr = L"argv[" + std::to_wstring(i) + L"]: " + argv[i] + L"\n";
				Logger::WriteMessage(argStr.c_str());
			}

			Logger::WriteMessage(L"About to call ParseCommandLine\n");

			try {
				auto options = ParseCommandLine(argc, argv);
				Logger::WriteMessage(L"ParseCommandLine completed\n");

				Logger::WriteMessage((L"targetPath: " + options.targetPath + L"\n").c_str());
				Logger::WriteMessage((L"detectMode: " +
					std::wstring(options.detectMode.begin(), options.detectMode.end()) + L"\n").c_str());
				Logger::WriteMessage((L"selectionMethod: " +
					std::to_wstring(static_cast<int>(options.selectionMethod)) + L"\n").c_str());

				Assert::AreEqual(L"TestExecutable.exe", options.targetPath.c_str());  // Updated assertion
				Assert::AreEqual(static_cast<int>(CommandLineOptions::SelectionMethod::DETECT),
					static_cast<int>(options.selectionMethod));
				Assert::AreEqual(L"P", options.detectMode.c_str());
			}
			catch (const std::exception& e) {
				Logger::WriteMessage(L"Exception caught: ");
				Logger::WriteMessage(std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
				Logger::WriteMessage(L"\n");
				throw;
			}
			catch (...) {
				Logger::WriteMessage(L"Unknown exception caught\n");
				throw;
			}

			CleanupArgs(argv);
		}

		TEST_METHOD(TestDirectCoreSpecification)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"TestExecutable.exe",
				L"--cores", L"0,1,2"
				});

			auto options = ParseCommandLine(argc, argv);

			Assert::AreEqual(L"TestExecutable.exe", options.targetPath.c_str());
			Assert::AreEqual(static_cast<int>(CommandLineOptions::SelectionMethod::DIRECT),
				static_cast<int>(options.selectionMethod));
			Assert::AreEqual(size_t(3), options.cores.size());
			Assert::AreEqual(0, options.cores[0]);
			Assert::AreEqual(1, options.cores[1]);
			Assert::AreEqual(2, options.cores[2]);
			CleanupArgs(argv);
		}

		TEST_METHOD(TestCustomDetectionWithCPUID)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"TestExecutable.exe",
				L"--detect", L"CUSTOM",
				L"--cpuid", L"40"
				});

			auto options = ParseCommandLine(argc, argv);

			Assert::AreEqual(L"TestExecutable.exe", options.targetPath.c_str());
			Assert::AreEqual(L"CUSTOM", options.detectMode.c_str());
			Assert::AreEqual(uint32_t(0x40), options.cpuidValue);
			CleanupArgs(argv);
		}

		TEST_METHOD(TestQueryMode)
		{
			auto [argc, argv] = PrepareArgs({ L"--query" });

			auto options = ParseCommandLine(argc, argv);

			Assert::IsTrue(options.queryMode);
			Assert::IsFalse(options.showHelp);
			CleanupArgs(argv);
		}

		TEST_METHOD(TestMissingTargetPath)
		{
			auto [argc, argv] = PrepareArgs({
				L"--detect", L"P"
				});

			Assert::ExpectException<std::runtime_error>([&]() {
				ParseCommandLine(argc, argv);
				});
			CleanupArgs(argv);
		}

		TEST_METHOD(TestInvalidCoreNumber)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"test.exe",
				L"--cores", L"0,999999"  // Assuming system doesn't have this many cores
				});

			Assert::ExpectException<std::runtime_error>([&]() {
				ParseCommandLine(argc, argv);
				});
			CleanupArgs(argv);
		}

		TEST_METHOD(TestDuplicateCores)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"test.exe",
				L"--cores", L"0,1,1"
				});

			Assert::ExpectException<std::runtime_error>([&]() {
				ParseCommandLine(argc, argv);
				});
			CleanupArgs(argv);
		}

		TEST_METHOD(TestInvertWithPCoreDetection)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"TestExecutable.exe",
				L"--detect", L"P",
				L"--invert"
				});

			auto options = ParseCommandLine(argc, argv);

			Assert::IsTrue(options.invertSelection);
			Assert::AreEqual(L"P", options.detectMode.c_str());
			CleanupArgs(argv);
		}

		TEST_METHOD(TestLoggingOptions)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"TestExecutable.exe",
				L"--detect", L"P",
				L"--log",
				L"--logpath", L"test.log"
				});

			auto options = ParseCommandLine(argc, argv);

			Assert::IsTrue(options.enableLogging);
			Assert::AreEqual(L"test.log", options.logPath.c_str());
			CleanupArgs(argv);
		}

		TEST_METHOD(TestInvertOption)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"TestExecutable.exe",
				L"--detect", L"P",
				L"--invert"
				});

			auto options = ParseCommandLine(argc, argv);

			Assert::IsTrue(options.invertSelection);
			CleanupArgs(argv);
		}

		TEST_METHOD(TestUnknownOption)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"TestExecutable.exe",
				L"--unknown-option"
				});

			Assert::ExpectException<std::runtime_error>([&]() {
				ParseCommandLine(argc, argv);
				}, L"Should throw on unknown option");
			CleanupArgs(argv);
		}

		TEST_METHOD(TestMissingArgumentValue)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target"  // Missing value
				});

			Assert::ExpectException<std::runtime_error>([&]() {
				ParseCommandLine(argc, argv);
				}, L"Should throw when argument value is missing");
			CleanupArgs(argv);
		}

		TEST_METHOD(TestShortFormWithLogging)
		{
			auto [argc, argv] = PrepareArgs({
				L"-t", L"TestExecutable.exe",
				L"-d", L"P",
				L"-l",
				L"--logpath", L"test.log"
				});

			auto options = ParseCommandLine(argc, argv);

			Assert::IsTrue(options.enableLogging);
			Assert::AreEqual(L"test.log", options.logPath.c_str());
			CleanupArgs(argv);
		}

		TEST_METHOD(TestMultipleValidOptions)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"TestExecutable.exe",
				L"--detect", L"CUSTOM",
				L"--cpuid", L"40",
				L"--invert",
				L"--log",
				L"--logpath", L"test.log"
				});

			auto options = ParseCommandLine(argc, argv);

			Assert::AreEqual(L"TestExecutable.exe", options.targetPath.c_str());
			Assert::AreEqual(static_cast<int>(CommandLineOptions::SelectionMethod::DETECT),
				static_cast<int>(options.selectionMethod));
			Assert::AreEqual(L"CUSTOM", options.detectMode.c_str());
			Assert::AreEqual(uint32_t(0x40), options.cpuidValue);
			Assert::IsTrue(options.invertSelection);
			Assert::IsTrue(options.enableLogging);
			Assert::AreEqual(L"test.log", options.logPath.c_str());
			CleanupArgs(argv);
		}

		TEST_METHOD(TestMalformedCoresList)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"TestExecutable.exe",
				L"--cores", L"0,1,abc"
				});

			Assert::ExpectException<std::runtime_error>([&]() {
				ParseCommandLine(argc, argv);
				}, L"Should throw on malformed cores list");
			CleanupArgs(argv);
		}

		TEST_METHOD(TestEmptyCoresList)
		{
			auto [argc, argv] = PrepareArgs({
				L"--target", L"TestExecutable.exe",
				L"--cores", L""
				});

			auto options = ParseCommandLine(argc, argv);
			Assert::AreEqual(size_t(0), options.cores.size());
			CleanupArgs(argv);
		}

		TEST_METHOD(TestHelpOptions)
		{
			// Test all help variations
			std::vector<std::wstring> helpOptions = { L"--help", L"-h", L"-?", L"/?" };

			for (const auto& helpOpt : helpOptions) {
				auto [argc, argv] = PrepareArgs({ helpOpt });

				Logger::WriteMessage((L"Testing help option: " + helpOpt + L"\n").c_str());
				auto options = ParseCommandLine(argc, argv);

				Assert::IsTrue(options.showHelp, (L"Help option failed: " + helpOpt).c_str());
				CleanupArgs(argv);
			}
		}

	};
}
