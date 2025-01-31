//CoreAwareProcessLauncher.Tests.cpp
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
            Assert::AreEqual(static_cast<int>(CommandLineOptions::CoreAffinityMode::CUSTOM),
                static_cast<int>(opts.affinityMode));
            Assert::AreEqual(uint32_t(0), opts.pattern);
            Assert::IsTrue(opts.cores.empty());
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

        TEST_METHOD(TestBasicTargetWithPCoreMode)
        {
            Logger::WriteMessage(L"Starting TestBasicTargetWithPCoreMode\n");

            auto [argc, argv] = PrepareArgs({
                L"--target", L"TestExecutable.exe",
                L"--mode", L"p"
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

                Assert::AreEqual(L"TestExecutable.exe", options.targetPath.c_str());
                Assert::AreEqual(static_cast<int>(CommandLineOptions::CoreAffinityMode::P_CORES_ONLY),
                    static_cast<int>(options.affinityMode));
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
            Assert::AreEqual(static_cast<int>(CommandLineOptions::CoreAffinityMode::CUSTOM),
                static_cast<int>(options.affinityMode));
            Assert::AreEqual(size_t(3), options.cores.size());
            Assert::AreEqual(0, options.cores[0]);
            Assert::AreEqual(1, options.cores[1]);
            Assert::AreEqual(2, options.cores[2]);
            CleanupArgs(argv);
        }

        TEST_METHOD(TestPatternDetection)
        {
            auto [argc, argv] = PrepareArgs({
                L"--target", L"TestExecutable.exe",
                L"--pattern", L"40"
                });

            auto options = ParseCommandLine(argc, argv);

            Assert::AreEqual(L"TestExecutable.exe", options.targetPath.c_str());
            Assert::AreEqual(static_cast<int>(CommandLineOptions::CoreAffinityMode::PATTERN),
                static_cast<int>(options.affinityMode));
            Assert::AreEqual(uint32_t(0x40), options.pattern);
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
                L"--mode", L"p"
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

        TEST_METHOD(TestInvertWithMode)
        {
            auto [argc, argv] = PrepareArgs({
                L"--target", L"TestExecutable.exe",
                L"--mode", L"p",
                L"--invert"
                });

            auto options = ParseCommandLine(argc, argv);

            Assert::IsTrue(options.invertSelection);
            Assert::AreEqual(static_cast<int>(CommandLineOptions::CoreAffinityMode::P_CORES_ONLY),
                static_cast<int>(options.affinityMode));
            CleanupArgs(argv);
        }

        TEST_METHOD(TestAllECoresMode)
        {
            auto [argc, argv] = PrepareArgs({
                L"--target", L"TestExecutable.exe",
                L"--mode", L"alle"
                });

            auto options = ParseCommandLine(argc, argv);

            Assert::AreEqual(static_cast<int>(CommandLineOptions::CoreAffinityMode::ALL_E_CORES),
                static_cast<int>(options.affinityMode));
            CleanupArgs(argv);
        }

        TEST_METHOD(TestLPCoresMode)
        {
            auto [argc, argv] = PrepareArgs({
                L"--target", L"TestExecutable.exe",
                L"--mode", L"lp"
                });

            auto options = ParseCommandLine(argc, argv);

            Assert::AreEqual(static_cast<int>(CommandLineOptions::CoreAffinityMode::LP_CORES_ONLY),
                static_cast<int>(options.affinityMode));
            CleanupArgs(argv);
        }

        TEST_METHOD(TestAllCoresMode)
        {
            auto [argc, argv] = PrepareArgs({
                L"--target", L"TestExecutable.exe",
                L"--mode", L"all"
                });

            auto options = ParseCommandLine(argc, argv);

            Assert::AreEqual(static_cast<int>(CommandLineOptions::CoreAffinityMode::ALL_CORES),
                static_cast<int>(options.affinityMode));
            CleanupArgs(argv);
        }

        TEST_METHOD(TestInvalidMode)
        {
            auto [argc, argv] = PrepareArgs({
                L"--target", L"TestExecutable.exe",
                L"--mode", L"invalid"
                });

            Assert::ExpectException<std::runtime_error>([&]() {
                ParseCommandLine(argc, argv);
                }, L"Should throw on invalid mode");
            CleanupArgs(argv);
        }
    };
}
