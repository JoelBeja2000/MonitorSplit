// MainViewModelTests.cs — Unit tests for MainViewModel
using FluentAssertions;
using NSubstitute;
using MonitorSplit.Models;
using MonitorSplit.Services;
using MonitorSplit.ViewModels;

namespace MonitorSplit.Tests;

public class MainViewModelTests
{
    private static DriverManager CreateMockDriverManager(bool installed = false, bool sendConfigOk = true)
    {
        // DriverManager has non-virtual members — we test the ViewModel
        // by injecting a real DriverManager but mock the underlying P/Invoke.
        // Since P/Invoke can't be mocked directly, we test the ViewModel
        // behaviors that don't depend on an actual driver being installed.
        return new DriverManager();
    }

    // ─── Split Ratio ─────────────────────────────────────────────────

    [Fact]
    public void SplitRatioSlider_Set50_UpdatesConfigAndLabel()
    {
        var vm = new MainViewModel(new DriverManager());

        vm.SplitRatioSlider = 50;

        vm.Config.SplitRatio.Should().BeApproximately(0.5, 0.001);
        vm.SplitRatioPercent.Should().Be("50%");
    }

    [Fact]
    public void SplitRatioSlider_Set70_UpdatesConfigTo70Percent()
    {
        var vm = new MainViewModel(new DriverManager());
        vm.Config.PhysicalWidth = 1920;
        vm.Config.PhysicalHeight = 1080;

        vm.SplitRatioSlider = 70;

        vm.Config.SplitRatio.Should().BeApproximately(0.70, 0.001);
        vm.SplitRatioPercent.Should().Be("70%");
    }

    [Theory]
    [InlineData(10, "10%")]
    [InlineData(50, "50%")]
    [InlineData(90, "90%")]
    public void SplitRatioPercent_ReturnsCorrectString(double sliderValue, string expected)
    {
        var vm = new MainViewModel(new DriverManager());
        vm.SplitRatioSlider = sliderValue;
        vm.SplitRatioPercent.Should().Be(expected);
    }

    // ─── Initial State ────────────────────────────────────────────────

    [Fact]
    public void OnStartup_StatusMessageIsReady()
    {
        var vm = new MainViewModel(new DriverManager());
        // Status message should be set during startup
        vm.StatusMessage.Should().NotBeNullOrEmpty();
    }

    [Fact]
    public void OnStartup_ConfigHasVersion1()
    {
        var vm = new MainViewModel(new DriverManager());
        vm.Config.Version.Should().Be(SplitConfig.CurrentVersion);
    }

    // ─── Command Availability ─────────────────────────────────────────

    [Fact]
    public void EnableSplitCommand_WhenDriverNotInstalled_SetsErrorMessage()
    {
        var vm = new MainViewModel(new DriverManager());
        // Driver is not installed in test environment
        vm.IsDriverInstalled.Should().BeFalse();

        vm.EnableSplitCommand.Execute(null);

        vm.StatusMessage.Should().Contain("not installed");
    }

    [Fact]
    public void DisableSplitCommand_AlwaysAvailable()
    {
        var vm = new MainViewModel(new DriverManager());
        vm.DisableSplitCommand.CanExecute(null).Should().BeTrue();
    }

    [Fact]
    public void RefreshMonitorsCommand_ExecutesWithoutException()
    {
        var vm = new MainViewModel(new DriverManager());
        var act = () => vm.RefreshMonitorsCommand.Execute(null);
        act.Should().NotThrow();
    }

    // ─── Monitor Selection ────────────────────────────────────────────

    [Fact]
    public void SelectingMonitor_UpdatesConfigDimensions()
    {
        var vm = new MainViewModel(new DriverManager());

        var monitor = new PhysicalMonitor
        {
            DeviceName   = "\\\\.\\DISPLAY1",
            FriendlyName = "Test Monitor",
            Width        = 2560,
            Height       = 1440,
            RefreshHz    = 144,
            IsPrimary    = true
        };

        vm.SelectedMonitor = monitor;

        vm.Config.PhysicalWidth.Should().Be(2560);
        vm.Config.PhysicalHeight.Should().Be(1440);
        vm.Config.Monitor1RefreshHz.Should().Be(144);
        vm.Config.Monitor2RefreshHz.Should().Be(144);
    }
}
