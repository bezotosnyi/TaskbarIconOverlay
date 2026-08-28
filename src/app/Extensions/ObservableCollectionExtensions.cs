using System.Collections.ObjectModel;

namespace TaskbarIconOverlay.App.Extensions;

public static class ObservableCollectionExtensions
{
    public static void Swap<T>(
        this ObservableCollection<T> collection,
        int firstIndex,
        int secondIndex)
    {
        if (firstIndex == secondIndex)
            return;

        // Avoid deconstruction pattern to trigger collection change events
        var temp = collection[firstIndex];
        collection[firstIndex] = collection[secondIndex];
        collection[secondIndex] = temp;
    }
}
