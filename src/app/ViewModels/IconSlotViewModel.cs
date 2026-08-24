using System;
using System.IO;
using System.Windows.Media.Imaging;

namespace TaskbarIconOverlay.App.ViewModels;

public sealed class IconSlotViewModel : ViewModelBase
{
    private string _imagePath = "";
    private BitmapImage? _preview;
    private bool _hasNumber;
    private int _slotNumber;

    /// <summary>
    /// 1-based position - display only, recalculated by the parent
    /// whenever the collection is reordered (drag-drop) or resized.
    /// </summary>
    public int SlotNumber
    {
        get => _slotNumber;
        set => SetField(ref _slotNumber, value);
    }

    public string ImagePath
    {
        get => _imagePath;
        set
        {
            if (!SetField(ref _imagePath, value)) return;
            RefreshPreview();
        }
    }

    public BitmapImage? Preview
    {
        get => _preview;
        private set => SetField(ref _preview, value);
    }

    /// <summary>
    /// Set by the parent ViewModel whenever NumberedCount changes
    /// or this slot's position changes - not user-editable directly.
    /// </summary>
    public bool HasNumber
    {
        get => _hasNumber;
        set => SetField(ref _hasNumber, value);
    }

    private void RefreshPreview()
    {
        if (string.IsNullOrEmpty(ImagePath) || !File.Exists(ImagePath))
        {
            Preview = null;
            return;
        }
        try
        {
            var bmp = new BitmapImage();
            bmp.BeginInit();
            bmp.CacheOption = BitmapCacheOption.OnLoad;
            bmp.UriSource = new Uri(ImagePath);
            bmp.EndInit();
            bmp.Freeze();
            Preview = bmp;
        }
        catch
        {
            Preview = null;
        }
    }
}
