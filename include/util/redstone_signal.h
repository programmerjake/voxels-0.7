#ifndef REDSTONE_SIGNAL_H_INCLUDED
#define REDSTONE_SIGNAL_H_INCLUDED

namespace programmerjake
{
namespace voxels
{
struct RedstoneSignal final
{
private:
    static constexpr int max(int a, int b)
    {
        return a > b ? a : b;
    }
public:
    int signalStrength = 0;
    int redstoneDustSignalStrength = 0;
    bool canConnectToRedstoneDustDirectly = false;
    bool canConnectToRedstoneDustThroughBlock = false;
    static constexpr int maxSignalStrength = 0xF;
    constexpr RedstoneSignal()
    {
    }
    constexpr RedstoneSignal(int signalStrength, int redstoneDustSignalStrength, bool canConnectToRedstoneDustDirectly, bool canConnectToRedstoneDustThroughBlock)
        : signalStrength(max(signalStrength, redstoneDustSignalStrength)),
          redstoneDustSignalStrength(redstoneDustSignalStrength),
          canConnectToRedstoneDustDirectly(canConnectToRedstoneDustDirectly || canConnectToRedstoneDustThroughBlock),
          canConnectToRedstoneDustThroughBlock(canConnectToRedstoneDustThroughBlock)
    {
    }
    bool good() const
    {
        return redstoneDustSignalStrength != 0 || canConnectToRedstoneDustDirectly;
    }
    constexpr RedstoneSignal combine(RedstoneSignal rt) const
    {
        return RedstoneSignal(max(signalStrength, rt.signalStrength),
                              max(redstoneDustSignalStrength, rt.redstoneDustSignalStrength),
                              canConnectToRedstoneDustDirectly || rt.canConnectToRedstoneDustDirectly,
                              canConnectToRedstoneDustThroughBlock || rt.canConnectToRedstoneDustThroughBlock);
    }
    constexpr RedstoneSignal transmitThroughSolidBlock() const
    {
        return RedstoneSignal(signalStrength, 0, canConnectToRedstoneDustThroughBlock, false);
    }
    constexpr int getRedstoneDustSignalStrength() const
    {
        return canConnectToRedstoneDustDirectly ? max(signalStrength, redstoneDustSignalStrength - 1) : 0;
    }
    constexpr bool isOnAtAll() const
    {
        return signalStrength > 0;
    }
};
}
}

#endif // REDSTONE_SIGNAL_H_INCLUDED
