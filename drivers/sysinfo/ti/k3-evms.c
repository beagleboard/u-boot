// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019-2021 Texas Instruments Incorporated - https://www.ti.com/
 *     Jean-Jacques Hiblot <jjhiblot@ti.com>
 */

#include <common.h>
#include <dm.h>
#include <dm/devres.h>
#include <image.h>
#include <sysinfo.h>
#include <linux/ctype.h>

extern char *k3_dtbo_list[];

struct sysinfo_k3_priv {
	int nb_daughter_boards;
	char **names;
};

static const struct udevice_id sysinfo_k3_ids[] = {
	{ .compatible = "ti,am654-evm",},
	{ .compatible = "ti,j721e-evm",},
	{ /* sentinel */ }
};

static int sysinfo_k3_detect(struct udevice *dev)
{
	struct sysinfo_k3_priv *priv = dev_get_priv(dev);
	int i, count = 0;

	while (k3_dtbo_list[count])
		count++;

	priv->nb_daughter_boards = count;
	priv->names = devm_kmalloc(dev, sizeof(*priv->names) * count, 0);
	for (i = 0; i < priv->nb_daughter_boards; i++) {
		int len = strlen(k3_dtbo_list[i]);

		/* remove the ".dtbo" suffix */
		len -= 5;
		priv->names[i] = devm_kmalloc(dev, len, 0);
		memcpy(priv->names[i], k3_dtbo_list[i], len);
		priv->names[i][len] = '\0';
	}

	return 0;
}

static int sysinfo_k3_get_fit_loadable(struct udevice *dev, int index,
				       const char *type, const char **strp)
{
	struct sysinfo_k3_priv *priv = dev_get_priv(dev);

	if (strcmp(type, FIT_FDT_PROP))
		return -ENOENT;

	if (index >= priv->nb_daughter_boards)
		return -ENOENT;

	*strp = priv->names[index];

	return 0;
}

static const struct sysinfo_ops sysinfo_k3_ops = {
	.detect = sysinfo_k3_detect,
	.get_fit_loadable = sysinfo_k3_get_fit_loadable,
	.get_int = NULL,
	.get_str = NULL,
};

U_BOOT_DRIVER(board_k3_evms) = {
	.name           = "board_k3_evms",
	.id             = UCLASS_SYSINFO,
	.of_match       = sysinfo_k3_ids,
	.ops		= &sysinfo_k3_ops,
	.priv_auto_alloc_size = sizeof(struct sysinfo_k3_priv),
};
