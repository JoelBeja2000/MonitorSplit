// Converters.cs — WPF value converters for data binding
using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace MonitorSplit.Converters;

/// <summary>Inverts a boolean value for binding (True → False, False → True).</summary>
public class InverseBoolConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        => value is bool b ? !b : false;
    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => value is bool b ? !b : false;
}

/// <summary>Converts null to Collapsed, non-null to Visible.</summary>
public class NullToCollapsedConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        => value == null ? Visibility.Collapsed : Visibility.Visible;
    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotImplementedException();
}

/// <summary>
/// Converts a split ratio (0–100) to a GridLength that WPF can use.
/// Used to dynamically resize the monitor preview columns.
/// </summary>
public class RatioToGridLengthConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is double ratio && ratio is >= 1 and <= 99)
            return new GridLength(ratio, GridUnitType.Star);
        return new GridLength(50, GridUnitType.Star);
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotImplementedException();
}

/// <summary>Converts a boolean IsEnabled to an activation badge text.</summary>
public class BoolToActivationConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        => value is true ? "Active" : "Inactive";
    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotImplementedException();
}
