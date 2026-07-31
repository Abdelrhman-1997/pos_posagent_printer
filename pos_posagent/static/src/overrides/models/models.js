/** @odoo-module */

import { PosStore } from "@point_of_sale/app/store/pos_store";
import { patch } from "@web/core/utils/patch";

patch(PosStore.prototype, {
    // Odoo 17 called this hook `after_load_server_data`. Odoo 18 renamed
    // it to `afterProcessServerData` and runs it before proxy connection.
    async afterProcessServerData() {
        const result = await super.afterProcessServerData(...arguments);
        if (this.config.use_posagent && this.config.posagent_enable_printer) {
            this.config.is_posbox = true;
            this.config.iface_print_via_proxy = true;
            this.config.iface_cashdrawer = Boolean(this.config.posagent_enable_cashdrawer);
            this.config.iface_scan_via_proxy = false;
            this.config.iface_electronic_scale = false;
            this.config.proxy_ip = `http://127.0.0.1:${this.config.pos_agent_port}`;
        }
        return result;
    },
});
