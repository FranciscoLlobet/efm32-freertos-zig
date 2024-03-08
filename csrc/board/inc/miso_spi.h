/**
 * @file miso_spi.h
 * @author Francisco
 * @brief
 * @date 2024-03-08
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef __MISO_SPI_H__
#define __MISO_SPI_H__

#include "miso.h"

/**
 * @brief Miso Generic SPI error
 */
#define MISO_SPI_ERROR ((ssize_t)-127)

/**
 * @brief Forward declaration of miso spi device
 *
 */
typedef struct miso_spi_dev_s* miso_spi_t;

/**
 * @brief Miso SPI send data
 *
 * @param dev Miso SPI device
 *
 * @param data Data to send
 *
 * @param size Size of data to send
 *
 * @return Number of bytes sent. Negative value on error
 */
ssize_t miso_spi_send(miso_spi_t dev, const void* data, size_t nBytes);

/**
 * @brief Miso SPI receive data
 *
 *
 */
ssize_t miso_spi_receive(miso_spi_t dev, void* data, size_t nBytes);

#endif  // __MISO_SPI_H__