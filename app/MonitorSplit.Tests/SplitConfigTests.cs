// SplitConfigTests.cs — Unit tests for the SplitConfig model
using FluentAssertions;
using MonitorSplit.Models;

namespace MonitorSplit.Tests;

public class SplitConfigTests
{
    // ─── RecalculateFromPhysical ──────────────────────────────────────

    [Fact]
    public void RecalculateFromPhysical_50Percent_SplitsBothHalvesEqually()
    {
        var config = new SplitConfig
        {
            PhysicalWidth  = 1920,
            PhysicalHeight = 1080,
            SplitRatio     = 0.5
        };

        config.RecalculateFromPhysical();

        config.Monitor1Width.Should().Be(960);
        config.Monitor1Height.Should().Be(1080);
        config.Monitor2Width.Should().Be(960);
        config.Monitor2Height.Should().Be(1080);
    }

    [Fact]
    public void RecalculateFromPhysical_67Percent_SplitsCorrectly()
    {
        var config = new SplitConfig
        {
            PhysicalWidth  = 2560,
            PhysicalHeight = 1440,
            SplitRatio     = 0.67
        };

        config.RecalculateFromPhysical();

        config.Monitor1Width.Should().Be((int)(2560 * 0.67));
        config.Monitor2Width.Should().Be(2560 - (int)(2560 * 0.67));
        config.Monitor1Height.Should().Be(1440);
        config.Monitor2Height.Should().Be(1440);
    }

    [Fact]
    public void RecalculateFromPhysical_SumOfWidthsEqualsPhysical()
    {
        var config = new SplitConfig
        {
            PhysicalWidth  = 3440,
            PhysicalHeight = 1440,
            SplitRatio     = 0.6
        };

        config.RecalculateFromPhysical();

        (config.Monitor1Width + config.Monitor2Width).Should().Be(config.PhysicalWidth);
    }

    [Fact]
    public void RecalculateFromPhysical_HeightIsSameForBothMonitors()
    {
        var config = new SplitConfig
        {
            PhysicalWidth  = 1920,
            PhysicalHeight = 1200,
            SplitRatio     = 0.3
        };

        config.RecalculateFromPhysical();

        config.Monitor1Height.Should().Be(config.PhysicalHeight);
        config.Monitor2Height.Should().Be(config.PhysicalHeight);
    }

    // ─── Default Values ───────────────────────────────────────────────

    [Fact]
    public void DefaultConfig_HasExpectedValues()
    {
        var config = new SplitConfig();

        config.Version.Should().Be(SplitConfig.CurrentVersion);
        config.SplitRatio.Should().Be(0.5);
        config.IsEnabled.Should().BeFalse();
        config.Monitor1Width.Should().Be(960);
        config.Monitor2Width.Should().Be(960);
        config.Monitor1RefreshHz.Should().Be(60);
    }

    // ─── Edge Cases ───────────────────────────────────────────────────

    [Theory]
    [InlineData(0.1)]
    [InlineData(0.5)]
    [InlineData(0.9)]
    public void RecalculateFromPhysical_ValidRatios_DoNotThrow(double ratio)
    {
        var config = new SplitConfig
        {
            PhysicalWidth  = 1920,
            PhysicalHeight = 1080,
            SplitRatio     = ratio
        };

        var act = () => config.RecalculateFromPhysical();

        act.Should().NotThrow();
        config.Monitor1Width.Should().BeGreaterThan(0);
        config.Monitor2Width.Should().BeGreaterThan(0);
    }

    [Fact]
    public void PhysicalMonitor_ToString_FormatsCorrectly()
    {
        var monitor = new PhysicalMonitor
        {
            FriendlyName = "Generic Monitor",
            Width        = 1920,
            Height       = 1080,
            RefreshHz    = 60,
            IsPrimary    = true
        };

        monitor.ToString().Should().Contain("1920");
        monitor.ToString().Should().Contain("1080");
        monitor.ToString().Should().Contain("60");
        monitor.ToString().Should().Contain("[Primary]");
    }

    [Fact]
    public void NonPrimaryMonitor_ToString_DoesNotContainPrimaryTag()
    {
        var monitor = new PhysicalMonitor
        {
            FriendlyName = "Secondary",
            Width        = 2560,
            Height       = 1440,
            RefreshHz    = 144,
            IsPrimary    = false
        };

        monitor.ToString().Should().NotContain("[Primary]");
    }
}
