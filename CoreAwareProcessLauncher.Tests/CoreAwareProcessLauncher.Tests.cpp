//CoreAwareProcessLauncher.Tests.cpp
#include "pch.h"
#include "options.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreAwareProcessLauncherTests
{
    TEST_CLASS(OptionsTests)
    {
    public:
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
    };
}