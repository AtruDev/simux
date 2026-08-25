/* Os controles de reprodução. O scrub é o que prova a arquitetura: arrastar
 * para trás não desfaz evento nenhum — reexecuta de zero até o passo pedido. */

import type { Player } from "../core/player";
import { t } from "../i18n";

const VELOCIDADES = [0.25, 0.5, 1, 2, 4, 8, 16];

interface Props {
  player: Player;
  i: number;
  total: number;
  tocando: boolean;
  velocidade: number;
}

export function Transporte({ player, i, total, tocando, velocidade }: Props) {
  const vazio = total === 0;

  return (
    <div className="transporte">
      <div className="botoes">
        <button
          type="button"
          className="icone"
          title={t("transporte.inicio")}
          aria-label={t("transporte.inicio")}
          disabled={vazio}
          onClick={() => player.irPara(0)}
        >
          ⏮
        </button>
        <button
          type="button"
          className="icone"
          title={t("transporte.anterior")}
          aria-label={t("transporte.anterior")}
          disabled={vazio || i === 0}
          onClick={() => player.passo(-1)}
        >
          ◀
        </button>
        <button
          type="button"
          className="icone principal"
          title={tocando ? t("transporte.pausar") : t("transporte.tocar")}
          aria-label={tocando ? t("transporte.pausar") : t("transporte.tocar")}
          disabled={vazio}
          onClick={() => player.alternar()}
        >
          {tocando ? "❚❚" : "▶"}
        </button>
        <button
          type="button"
          className="icone"
          title={t("transporte.proximo")}
          aria-label={t("transporte.proximo")}
          disabled={vazio || i >= total}
          onClick={() => player.passo(1)}
        >
          ▶|
        </button>
        <button
          type="button"
          className="icone"
          title={t("transporte.fim")}
          aria-label={t("transporte.fim")}
          disabled={vazio}
          onClick={() => player.irPara(total)}
        >
          ⏭
        </button>
      </div>

      <input
        className="barra"
        type="range"
        min={0}
        max={Math.max(total, 1)}
        value={i}
        disabled={vazio}
        aria-label={t("transporte.passo")}
        onChange={(e) => {
          player.pause();
          player.irPara(Number(e.target.value));
        }}
      />

      <label className="velocidade">
        <span className="rotulo">{t("transporte.velocidade")}</span>
        <select
          value={velocidade}
          onChange={(e) => player.setVelocidade(Number(e.target.value))}
        >
          {VELOCIDADES.map((v) => (
            <option key={v} value={v}>
              {v}×
            </option>
          ))}
        </select>
      </label>

      <span className="contagem mono numero">
        {i} / {total}
      </span>
    </div>
  );
}
