#include "image.h"

std::shared_ptr<ImageBlock>
BlockManager::getBlock(int _x, int _y) const {
    return std::make_shared<ImageBlock>(
        Vector2i(_x * blockSize, _y * blockSize),
        Vector2i(blockSize)
    );
}