using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Windows.Media;
using System.Windows.Threading;
using TaskbarIconOverlay.App.Models;
using TaskbarIconOverlay.App.Services;

namespace TaskbarIconOverlay.App.ViewModels;

public sealed class MainViewModel : ViewModelBase
{
    // Debounce window: rapid successive changes (dragging a Slider, typing
    // into a bound field) are batched into a single SharedConfigWriter.Write
    // call after this pause, instead of one write per intermediate value.
    private static readonly TimeSpan ApplyDebounceInterval = TimeSpan.FromMilliseconds(150);

    private readonly IFileDialogService _fileDialogService;
    private readonly SharedConfigWriter _configWriter;
    private readonly DispatcherTimer _applyDebounceTimer;

    private bool _stickyIconBinding = true;
    private int _numberedCount = 10;
    private bool _allowNumbersBeyondTen;
    private NumberPosition _numberPosition = NumberPosition.TopLeft;
    private int _numberSize = 12;
    private Color _numberColor = Colors.White;
    private Color _backgroundColor = Color.FromArgb(0x80, 0, 0, 0);
    private bool _showOnAllTaskbars;
    private bool _isEnabled;

    public MainViewModel(IFileDialogService fileDialogService, SharedConfigWriter configWriter)
    {
        _fileDialogService = fileDialogService;
        _configWriter = configWriter;

        _applyDebounceTimer = new DispatcherTimer { Interval = ApplyDebounceInterval };
        _applyDebounceTimer.Tick += (_, _) => {
            _applyDebounceTimer.Stop();
            Apply();
        };

        Slots = new ObservableCollection<IconSlotViewModel>();
        Slots.CollectionChanged += (_, _) => RenumberSlots();

        // Default preset: 1 window, no image yet.
        Slots.Add(new IconSlotViewModel());
        Apply();

        AddSlotCommand = new RelayCommand(_ => AddSlot());
        RemoveSlotCommand = new RelayCommand(param => RemoveSlot(param as IconSlotViewModel));
        BrowseIconCommand = new RelayCommand(param => BrowseIcon(param as IconSlotViewModel));
        ApplyCommand = new RelayCommand(_ => Apply());
        ToggleEnabledCommand = new RelayCommand(_ => ToggleEnabled());
    }

    public ObservableCollection<IconSlotViewModel> Slots { get; }

    public bool StickyIconBinding
    {
        get => _stickyIconBinding;
        set { if (SetField(ref _stickyIconBinding, value)) ScheduleApply(); }
    }

    /// <summary>
    /// How many of the leading slots (by position) get a number
    /// overlay. Independent from Slots.Count - a slot can have an icon
    /// without a number if its position is beyond this count.
    /// </summary>
    public int NumberedCount
    {
        get => _numberedCount;
        set
        {
            var clamped = AllowNumbersBeyondTen ? Math.Max(0, value) : Math.Clamp(value, 0, 10);
            if (SetField(ref _numberedCount, clamped))
            {
                RenumberSlots();  // cheap, local - keep immediate so the
                                  // number badges in the slot grid update
                                  // live while dragging, even though the
                                  // actual IPC write is debounced below
                ScheduleApply();
            }
        }
    }

    public bool AllowNumbersBeyondTen
    {
        get => _allowNumbersBeyondTen;
        set
        {
            if (!SetField(ref _allowNumbersBeyondTen, value)) return;
            // Re-clamp NumberedCount under the new rule.
            NumberedCount = _numberedCount;
        }
    }

    public NumberPosition NumberPosition
    {
        get => _numberPosition;
        set { if (SetField(ref _numberPosition, value)) ScheduleApply(); }
    }

    /// <summary>
    /// 8-16 per the original mod's own constraint.
    /// </summary>
    public int NumberSize
    {
        get => _numberSize;
        set { if (SetField(ref _numberSize, Math.Clamp(value, 8, 16))) ScheduleApply(); }
    }

    public Color NumberColor
    {
        get => _numberColor;
        set { if (SetField(ref _numberColor, value)) ScheduleApply(); }
    }

    public Color BackgroundColor
    {
        get => _backgroundColor;
        set { if (SetField(ref _backgroundColor, value)) ScheduleApply(); }
    }

    public bool ShowOnAllTaskbars
    {
        get => _showOnAllTaskbars;
        set { if (SetField(ref _showOnAllTaskbars, value)) ScheduleApply(); }
    }

    public bool IsEnabled
    {
        get => _isEnabled;
        private set => SetField(ref _isEnabled, value);
    }

    public RelayCommand AddSlotCommand { get; }
    public RelayCommand RemoveSlotCommand { get; }
    public RelayCommand BrowseIconCommand { get; }
    public RelayCommand ApplyCommand { get; }
    public RelayCommand ToggleEnabledCommand { get; }

    private void AddSlot()
    {
        Slots.Add(new IconSlotViewModel());
        Apply();  // discrete click, not a drag burst - write immediately
    }

    private void RemoveSlot(IconSlotViewModel? slot)
    {
        if (slot is null) return;
        Slots.Remove(slot);
        Apply();
    }

    private void BrowseIcon(IconSlotViewModel? slot)
    {
        if (slot is null) return;

        var initialDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "icons");
        var picked = _fileDialogService.BrowseForImage(
            Directory.Exists(initialDir) ? initialDir : null);

        if (picked is null) return;  // user cancelled

        slot.ImagePath = picked;
        Apply();
    }

    /// <summary>
    /// Called whenever Slots changes size/order (add, remove, or
    /// drag-drop reorder) - recomputes each slot's display number and
    /// HasNumber flag from its live position, since Win+N numbering is
    /// purely positional.
    /// </summary>
    private void RenumberSlots()
    {
        for (var i = 0; i < Slots.Count; i++)
        {
            Slots[i].SlotNumber = i + 1;
            Slots[i].HasNumber = i < NumberedCount;
        }
    }

    private void ToggleEnabled()
    {
        IsEnabled = !IsEnabled;
        _configWriter.SetEnabled(IsEnabled);
    }

    /// <summary>
    /// Restarts the debounce timer - only the LAST call within
    /// ApplyDebounceInterval actually results in a write. DispatcherTimer
    /// runs on the UI thread, same thread every property setter above is
    /// called from, so no locking is needed around it.
    /// </summary>
    private void ScheduleApply()
    {
        _applyDebounceTimer.Stop();
        _applyDebounceTimer.Start();
    }

    private static uint ToArgb(Color c) =>
        ((uint)c.A << 24) | ((uint)c.R << 16) | ((uint)c.G << 8) | c.B;

    private void Apply()
    {
        RenumberSlots();

        _configWriter.Write(new SharedConfigData
        {
            IconPaths = Slots.Select(s => s.ImagePath).ToList(),
            StickyIconBinding = StickyIconBinding,
            NumberedCount = NumberedCount,
            AllowNumbersBeyondTen = AllowNumbersBeyondTen,
            NumberPosition = NumberPosition,
            NumberSize = NumberSize,
            NumberColorArgb = ToArgb(NumberColor),
            BackgroundColorArgb = ToArgb(BackgroundColor),
            ShowOnAllTaskbars = ShowOnAllTaskbars,
        });
    }
}
