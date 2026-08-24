using System;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using TaskbarIconOverlay.App.ViewModels;
using Point = System.Windows.Point;

namespace TaskbarIconOverlay.App.Views;

public partial class MainWindow : Window
{
    private Point _dragStartPoint;
    private IconSlotViewModel? _draggedSlot;

    public MainWindow()
    {
        InitializeComponent();
    }

    private MainViewModel ViewModel => (MainViewModel)DataContext;

    // --- Drag-reorder (WPF has no built-in ItemsControl drag-reorder -
    // this is the standard manual pattern: capture start position, start a
    // DragDrop operation once the mouse moves past a threshold, swap
    // positions in the ViewModel's collection on Drop.) ---

    private void SlotBorder_MouseDown(object sender, MouseButtonEventArgs e)
    {
        _dragStartPoint = e.GetPosition(null);
        if (sender is FrameworkElement { DataContext: IconSlotViewModel slot })
        {
            _draggedSlot = slot;
        }
    }

    private void SlotBorder_MouseMove(object sender, MouseEventArgs e)
    {
        if (e.LeftButton != MouseButtonState.Pressed || _draggedSlot is null) return;

        var current = e.GetPosition(null);
        if (Math.Abs(current.X - _dragStartPoint.X) < SystemParameters.MinimumHorizontalDragDistance &&
            Math.Abs(current.Y - _dragStartPoint.Y) < SystemParameters.MinimumVerticalDragDistance)
        {
            return;  // hasn't moved far enough to count as a drag yet
        }

        if (sender is FrameworkElement element)
        {
            DragDrop.DoDragDrop(element, _draggedSlot, DragDropEffects.Move);
        }
    }

    private void SlotBorder_Drop(object sender, DragEventArgs e)
    {
        if (_draggedSlot is null) return;
        if (sender is not FrameworkElement { DataContext: IconSlotViewModel targetSlot }) return;
        if (ReferenceEquals(_draggedSlot, targetSlot)) return;

        var slots = ViewModel.Slots;
        var oldIndex = slots.IndexOf(_draggedSlot);
        var newIndex = slots.IndexOf(targetSlot);
        if (oldIndex < 0 || newIndex < 0) return;

        slots.Move(oldIndex, newIndex);  // ObservableCollection.Move raises
                                         // CollectionChanged, which the
                                         // ViewModel already listens to
                                         // (RenumberSlots) - no extra
                                         // wiring needed here.
        _draggedSlot = null;
    }

    // --- Color pickers ---
    //
    // WPF has no built-in color picker dialog. Using
    // System.Windows.Forms.ColorDialog here (requires
    // <UseWindowsForms>true</UseWindowsForms> in the .csproj) is pragmatic
    // and common practice rather than building a custom picker control.
    // Kept in code-behind, not the ViewModel, for the same reason
    // IFileDialogService exists - dialog invocation is view-layer
    // mechanics, but unlike file browsing this one is simple/self-contained
    // enough that a full service abstraction isn't worth it here.

    private void NumberColorSwatch_MouseDown(object sender, MouseButtonEventArgs e)
    {
        var picked = PickColor(ViewModel.NumberColor);
        if (picked is { } color) ViewModel.NumberColor = color;
    }

    private void BackgroundColorSwatch_MouseDown(object sender, MouseButtonEventArgs e)
    {
        var picked = PickColor(ViewModel.BackgroundColor);
        if (picked is { } color) ViewModel.BackgroundColor = color;
    }

    private static Color? PickColor(Color initial)
    {
        using var dialog = new System.Windows.Forms.ColorDialog
        {
            Color = System.Drawing.Color.FromArgb(initial.A, initial.R, initial.G, initial.B),
            FullOpen = true,
        };

        if (dialog.ShowDialog() != System.Windows.Forms.DialogResult.OK) return null;

        var c = dialog.Color;
        return Color.FromArgb(c.A, c.R, c.G, c.B);
    }
}
