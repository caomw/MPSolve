/*
 * secular-ga.c
 *
 *  Created on: 15/giu/2011
 *      Author: leonardo
 */

#include <mps/core.h>
#include <mps/link.h>
#include <mps/secular.h>


/**
 * @brief Routine that performs a block of iteration
 * in floating point on the secular equation.
 *
 * @param s the pointer to the mps_status struct.
 * @param maxit Maximum number of iteration to perform.
 * @return The number of approximated roots after the iteration.
 */
int
mps_secular_ga_fiterate(mps_status* s, int maxit)
{
  MPS_DEBUG_THIS_CALL

  int computed_roots = 0;
  int iterations = 0;
  int i;

  /* Iterate with newton until we have good approximations
   * of the roots */
  /* Set again to true */
  for (i = 0; i < s->n; i++)
    s->again[i] = true;

  while (computed_roots < s->n && iterations < maxit - 1)
    {
      cplx_t corr, abcorr;
      double modcorr;

      /* Increase iterations counter */
      iterations++;

      for (i = 0; i < s->n; i++)
        {
          if (s->again[i])
            {
              mps_secular_fnewton(s, s->froot[i], &s->frad[i], corr,
                  &s->again[i]);

              /* Apply Aberth correction */
              mps_faberth(s, i, abcorr);
              cplx_mul_eq(abcorr, corr);
              cplx_sub(abcorr, cplx_one, abcorr);
              cplx_div(abcorr, corr, abcorr);
              cplx_sub_eq(s->froot[i], abcorr);

              /* Correct the radius */
              modcorr = cplx_mod(abcorr);
              s->frad[i] += modcorr;

              if (!s->again[i])
                computed_roots++;
            }
        }
    }

  /* Return the number of approximated roots */
  return computed_roots;
}

/**
 * @brief Routine that performs a block of iteration
 * in DPE on the secular equation.
 *
 * @param s the pointer to the mps_status struct.
 * @param maxit Maximum number of iteration to perform.
 * @return The number of approximated roots after the iteration.
 */
int
mps_secular_ga_diterate(mps_status* s, int maxit)
{
  MPS_DEBUG_THIS_CALL

  int computed_roots = 0;
  int iterations = 0;
  int i;

  /* Iterate with newton until we have good approximations
   * of the roots */
  /* Set again to true */
  for (i = 0; i < s->n; i++)
    s->again[i] = true;

  while (computed_roots < s->n && iterations < maxit - 1)
    {
      cdpe_t corr, abcorr;
      rdpe_t modcorr;

      /* Increase iterations counter */
      iterations++;

      for (i = 0; i < s->n; i++)
        {
          if (s->again[i])
            {
              mps_secular_dnewton(s, s->droot[i], s->drad[i], corr,
                  &s->again[i]);

              /* Apply Aberth correction */
              mps_daberth(s, i, abcorr);
              cdpe_mul_eq(abcorr, corr);
              cdpe_sub(abcorr, cdpe_one, abcorr);
              cdpe_div(abcorr, corr, abcorr);
              cdpe_sub_eq(s->droot[i], abcorr);

              /* Correct the radius */
              cdpe_mod(modcorr, abcorr);
              rdpe_add_eq(s->drad[i], modcorr);

              if (!s->again[i])
                computed_roots++;
            }
        }
    }

  /* Return the number of approximated roots */
  return computed_roots;
}

/**
 * @brief Routine that performs a block of iteration
 * in Multiprecision on the secular equation.
 *
 * @param s the pointer to the mps_status struct.
 * @param maxit Maximum number of iteration to perform.
 * @return The number of approximated roots after the iteration.
 */
int
mps_secular_ga_miterate(mps_status* s, int maxit)
{
  MPS_DEBUG_THIS_CALL

  int computed_roots = 0;
  int iterations = 0;
  int i;
  int nit = 0;

  mpc_t corr, abcorr;
  cdpe_t ctmp;
  rdpe_t modcorr, drad;

  rdpe_set_2dl(drad, 1.0, -s->prec_out);

  /* Init data with the right precision */
  mpc_init2(corr, s->mpwp);
  mpc_init2(abcorr, s->mpwp);

  /* Iterate with newton until we have good approximations
   * of the roots */
  /* Allocate again and set all to true */
  for (i = 0; i < s->n; i++)
    {
      if (rdpe_gt(s->drad[i], drad))
        s->again[i] = true;
      else
        {
          s->again[i] = false;
          computed_roots++;
        }
    }

  while (computed_roots < s->n && iterations < maxit)
    {
      /* Increase iterations counter */
      iterations++;

      for (i = 0; i < s->n; i++)
        {
          if (s->again[i])
            {
              nit++;
              mps_secular_mnewton(s, s->mroot[i], s->drad[i], corr,
                  &s->again[i]);

              /* Apply Aberth correction */
              mps_maberth(s, i, abcorr);
              mpc_mul_eq(abcorr, corr);
              mpc_ui_sub(abcorr, 1, 0, abcorr);
              mpc_div(abcorr, corr, abcorr);
              mpc_sub_eq(s->mroot[i], abcorr);

              /* Correct the radius */
              mpc_get_cdpe(ctmp, abcorr);
              cdpe_mod(modcorr, ctmp);
              rdpe_add_eq(s->drad[i], modcorr);

              if (!s->again[i])
                computed_roots++;
            }
        }
    }

  /* Deallocate multiprecision local variables */
  mpc_clear(abcorr);
  mpc_clear(corr);

  MPS_DEBUG(s, "Performed %d iterations", nit)

  /* Return the number of approximated roots */
  return computed_roots;
}

/**
 * @brief Regenerate \f$a_i\f$ and \f$b_i\f$ setting
 * \f$b_i = z_i\f$, i.e. the current root approximation
 * and recomputing \f$a_i\f$ accordingly.
 */
void
mps_secular_ga_regenerate_coefficients(mps_status* s)
{
  MPS_DEBUG_THIS_CALL

  cplx_t *old_b, *old_a;
  cdpe_t *old_db, *old_da;
  mpc_t *old_ma, *old_mb;
  mps_secular_equation *sec;
  int i, j;

  /* Declaration and initialization of the multprecision
   * variables that are used only in that case */
  mpc_t prod_b, sec_ev;
  mpc_t ctmp, btmp;

  mpc_init2(prod_b, s->mpwp);
  mpc_init2(sec_ev, s->mpwp);
  mpc_init2(ctmp, s->mpwp);
  mpc_init2(btmp, s->mpwp);

  sec = (mps_secular_equation*) s->user_data;

  switch (s->lastphase)
    {

  /* If we are in the float phase regenerate coefficients
   * starting from floating point */
  case float_phase:

    /* Allocate old_a and old_b */
    old_a = cplx_valloc(s->n);
    old_b = cplx_valloc(s->n);

    /* Copy the old coefficients, and set the new
     * b_i with the current roots approximations. */
    for (i = 0; i < s->n; i++)
      {
        cplx_set(old_a[i], sec->afpc[i]);
        cplx_set(old_b[i], sec->bfpc[i]);
        cplx_set(sec->bfpc[i], s->froot[i]);
      }

    /* Compute the new a_i */
    for (i = 0; i < s->n; i++)
      {
        cplx_t prod_b, sec_ev;
        cplx_t ctmp, btmp;

        cplx_set(prod_b, cplx_one);
        cplx_set(sec_ev, cplx_zero);

        for (j = 0; j < sec->n; j++)
          {
            /* Compute 1 / (b - old_b) */
            cplx_sub(btmp, sec->bfpc[i], old_b[j]);

            if (cplx_eq_zero(btmp))
              {
                for (i = 0; i < sec->n; i++)
                  {
                    cplx_set(sec->afpc[i], old_a[i]);
                    cplx_set(sec->bfpc[i], old_b[i]);

                    MPS_DEBUG(s, "Cannot regenerate coefficients, reusing old ones")

                    cplx_vfree(old_a);
                    cplx_vfree(old_b);
                    return;
                  }
              }

            cplx_inv(ctmp, btmp);

            /* Add a_j / (b_i - old_b_j) to sec_ev */
            cplx_mul_eq(ctmp, old_a[j]);
            cplx_add_eq(sec_ev, ctmp);

            /* Divide prod_b for
             * b_i - b_j if i \neq j and
             * for b_i - old_b_i.  */
            cplx_mul_eq(prod_b, btmp);
            if (i != j)
              {
                cplx_sub(ctmp, sec->bfpc[i], sec->bfpc[j]);
                cplx_div_eq(prod_b, ctmp);
              }
          }

        /* Compute the new a_i as sec_ev * prod_old_b / prod_b */
        cplx_sub_eq(sec_ev, cplx_one);
        cplx_mul(sec->afpc[i], sec_ev, prod_b);
      }

    /* Free data */
    cplx_vfree(old_a);
    cplx_vfree(old_b);

    //    for(i = 0; i < s->n; i++)
    //      {
    //        MPS_DEBUG_CPLX(s, sec->afpc[i], "sec->afpc[%d]", i);
    //        MPS_DEBUG_CPLX(s, sec->bfpc[i], "sec->bfpc[%d]", i);
    //      }

    mps_secular_fstart(s, s->n, 0, 0, 0, s->eps_out);

    break;

    /* If this is the DPE phase regenerate DPE coefficients */
  case dpe_phase:

    /* Allocate old_a and old_b */
    old_da = cdpe_valloc(s->n);
    old_db = cdpe_valloc(s->n);

    /* Copy the old coefficients, and set the new
     * b_i with the current roots approximations. */
    for (i = 0; i < s->n; i++)
      {
        cdpe_set(old_da[i], sec->adpc[i]);
        cdpe_set(old_db[i], sec->bdpc[i]);
        cdpe_set(sec->bdpc[i], s->droot[i]);
      }

    /* Compute the new a_i */
    for (i = 0; i < s->n; i++)
      {
        cdpe_t prod_b, sec_ev;
        cdpe_t ctmp, btmp;
        cdpe_set(prod_b, cdpe_one);
        cdpe_set(sec_ev, cdpe_zero);

        for (j = 0; j < sec->n; j++)
          {
            /* Compute 1 / (b - old_b) */
            cdpe_sub(btmp, sec->bdpc[i], old_db[j]);

            /* Check if we can invert the difference, otherwisee abort
             * coefficient regeneration             */
            if (cdpe_eq_zero(btmp))
              {
                MPS_DEBUG(s, "Cannot regenerate coefficients, reusing old ones.")
                for(j = 0; j < sec->n; j++)
                  {
                    cdpe_set(sec->adpc[j], old_da[j]);
                    cdpe_set(sec->bdpc[j], old_db[j]);
                  }

                cdpe_vfree(old_da);
                cdpe_vfree(old_db);
                return;
              }

            cdpe_inv(ctmp, btmp);

            /* Add a_j / (b_i - old_b_j) to sec_ev */
            cdpe_mul_eq(ctmp, old_da[j]);
            cdpe_add_eq(sec_ev, ctmp);

            /* Multiply prod_b for
             * b_i - b_j if i \neq j and prod_old_b
             * for b_i - old_b_i.  */
            cdpe_mul_eq(prod_b, btmp);
            if (i != j)
              {
                cdpe_sub(ctmp, sec->bdpc[i], sec->bdpc[j]);
                cdpe_div_eq(prod_b, ctmp);
              }
          }

        /* Compute the new a_i as sec_ev * prod_old_b / prod_b */
        cdpe_sub_eq(sec_ev, cdpe_one);
        cdpe_mul(sec->adpc[i], sec_ev, prod_b);
      }

    /* Free data */
    cdpe_vfree(old_da);
    cdpe_vfree(old_db);

    /* Debug new coefficients found */
    for (i = 0; i < s->n; i++)
      {
        MPS_DEBUG_CDPE(s, sec->adpc[i], "sec->adpc[%d]", i);
      }

    mps_secular_dstart(s, s->n, 0, (__rdpe_struct *) rdpe_zero,
        (__rdpe_struct *) rdpe_zero, s->eps_out);

    break;

  case mp_phase:
    /* Allocate old_a and old_b */
    old_ma = mpc_valloc(s->n);
    old_mb = mpc_valloc(s->n);

    mpc_vinit2(old_ma, s->n, s->mpwp);
    mpc_vinit2(old_mb, s->n, s->mpwp);

    /* Copy the old coefficients, and set the new
     * b_i with the current roots approximations. */
    for (i = 0; i < s->n; i++)
      {
        mpc_set(old_ma[i], sec->ampc[i]);
        mpc_set(old_mb[i], sec->bmpc[i]);
        mpc_set(sec->bmpc[i], s->mroot[i]);
      }

    /* Compute the new a_i */
    for (i = 0; i < s->n; i++)
      {
        mpc_set_ui(prod_b, 1, 0);
        mpc_set_ui(sec_ev, 0, 0);

        for (j = 0; j < sec->n; j++)
          {
            /* Compute 1 / (b_i - old_b_j) */
            mpc_sub(btmp, sec->bmpc[i], old_mb[j]);

            /* If b - old_b is zero, simplify the computation */
            if (mpc_eq_zero(btmp))
              {
                for (i = 0; i < s->n; i++)
                  {
                    mpc_set(sec->bmpc[i], old_mb[i]);
                  }

                MPS_DEBUG(s, "Cannot regenerate coefficients, reusing old ones.")
                goto regenerate_m_exit;
              }

            mpc_inv(ctmp, btmp);

            /* Add a_j / (b_i - old_b_j) to sec_ev */
            mpc_mul_eq(ctmp, old_ma[j]);
            mpc_add_eq(sec_ev, ctmp);

            /* Multiply prod_b for
             * b_i - b_j if i \neq j and prod_old_b
             * for b_i - old_b_i.  */
            mpc_mul_eq(prod_b, btmp);
            if (i != j)
              {
                mpc_sub(ctmp, sec->bmpc[i], sec->bmpc[j]);
                mpc_div_eq(prod_b, ctmp);
              }
          }

        /* Compute the new a_i as sec_ev * prod_old_b / prod_b */
        mpc_sub_eq_ui(sec_ev, 1, 0);
        mpc_mul(sec->ampc[i], sec_ev, prod_b);
      }

    regenerate_m_exit:

    /* Free data */
    mpc_vclear(old_ma, s->n);
    mpc_vclear(old_mb, s->n);
    mpc_vfree(old_ma);
    mpc_vfree(old_mb);
    mpc_clear(prod_b);
    mpc_clear(sec_ev);
    mpc_clear(ctmp);
    mpc_clear(btmp);

    /* This is too much verbose to be enabled even in the debugged version,
     * so do not display it (unless we are trying to catch some errors on
     * coefficient regeneration). */
    /*
    for (i = 0; i < s->n; i++)
      {
        MPS_DEBUG_MPC(s, 15, sec->ampc[i], "sec->ampc[%d]", i);
        MPS_DEBUG_MPC(s, 15, sec->bmpc[i], "sec->bmpc[%d]", i);
      }
    */

        mps_secular_mstart(s, s->n, 0, (__rdpe_struct *) rdpe_zero,
            (__rdpe_struct *) rdpe_zero, s->eps_out);

    break;

  default:
    break;

    } /* End of switch (s->lastphase)*/

  /* Finally set radius according to new computed a_i coefficients,
   * if they are convenient   */
  mps_secular_set_radii(s);
}


/*
 * @brief Check if iterations can terminate
 */
mps_boolean
mps_secular_ga_check_stop(mps_status* s)
{
  MPS_DEBUG_THIS_CALL

  double frad = pow(10, -s->prec_out);
  rdpe_t drad;
  int i;

  /* Set the maximum rad allowed */
  rdpe_set_2dl(drad, 1.0, -s->prec_out);

  /* If we are in floating point and output precision is higher of
   * what is reachable, standing in floating point, do not exit */
  if (frad == 0 && s->lastphase == float_phase)
    return false;

  for (i = 0; i < s->n; i++)
    {
      switch (s->lastphase)
        {

      /* Float case */
      case float_phase:
        if (s->frad[i] > frad)
          return false;

      /* Multiprecision and DPE case are the same, since the radii
       * are always RDPE. */
      case mp_phase:
      case dpe_phase:
        if (rdpe_gt(s->drad[i], drad))
            return false;
        break;

      default:
        break;

        }
    }

  return true;
}

/**
 * @brief MPSolve main function for the secular equation solving
 * using Gemignani's approach.
 */
void
mps_secular_ga_mpsolve(mps_status* s, mps_phase phase)
{
  int roots_computed = 0;
  int iteration_per_packet = 10;
  int packet = 0;
  int i;

  for (i = 0; i < s->n; i++)
    s->frad[i] = DBL_MAX;

  /* Set initial cluster structure as no cluster structure. */
  mps_cluster_reset(s);

  /* Set phase */
  s->lastphase = phase;

  /* Select initial approximations using the custom secular
   * routine and based on the phase selected by the user. */
  switch (s->lastphase)
    {
  case float_phase:
    mps_secular_fstart(s, s->n, 0, 0.0, 0.0, s->eps_out);
    break;

  case dpe_phase:
    mps_secular_dstart(s, s->n, 0, (__rdpe_struct *) rdpe_zero,
        (__rdpe_struct *) rdpe_zero, s->eps_out);
    break;

  case mp_phase:
    mps_secular_mstart(s, s->n, 0, (__rdpe_struct *) rdpe_zero,
        (__rdpe_struct *) rdpe_zero, s->eps_out);
    break;

  default:
    break;
    }

  /* Set initial radius */
  mps_secular_set_radii(s);

  /* Cycle until approximated */
  do
    {
      /* Perform an iteration of floating point Aberth method */
      switch (s->lastphase)
        {
      case float_phase:
        roots_computed = mps_secular_ga_fiterate(s, iteration_per_packet);
        packet++;
        MPS_DEBUG(s, "%d roots were computed", roots_computed)
        break;

      case dpe_phase:
        roots_computed = mps_secular_ga_diterate(s, iteration_per_packet);
        packet++;
        MPS_DEBUG(s, "%d roots were computed", roots_computed)
        break;

      case mp_phase:
        roots_computed = mps_secular_ga_miterate(s, iteration_per_packet);
        MPS_DEBUG(s, "%d roots were computed", roots_computed)
        break;

      default:
        break;
        }

      /* Check if all roots were approximated with the
       * given input precision                      */
      if (mps_secular_ga_check_stop(s))
        return;

      /* Check if it's time to abandon floating point to enter
       * the multiprecision phase */
      if (s->lastphase != mp_phase &&
          ((roots_computed == s->n) || packet > 3))
        {
          MPS_DEBUG(s, "Switching to multiprecision phase")
          mps_secular_switch_phase(s, mp_phase);

          /* Regenerate coefficients is able to understand the type
           * of data that we are treating, so no switch is necessary
           * in here. */
          mps_secular_ga_regenerate_coefficients(s);
        }
      else if (s->lastphase == mp_phase)
        {
          /* If all the roots were approximated and we are in the multiprecision
           * phase then it's time to increase the precision, or stop if enough
           * precision has been reached. */
          mps_secular_raise_precision(s);

          /* Regenerate coefficients to accelerate convergence. */
          mps_secular_ga_regenerate_coefficients(s);
        }
    }
  while (!mps_secular_ga_check_stop(s));
}